#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#include "thread.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


#define ARGS_SIZE 5

pthread_mutex_t m;
pthread_cond_t cond_is_pending_request_empty;
pthread_cond_t cond_is_buffer_available;
pthread_cond_t cond_is_woerker_threads_and_pending_requests_empty;

Queue* workerThreadsQueue;
Queue* pendingRequestsQueue;
thread_stats* threadsStats;

enum SCHEDULER_ALGORITHM {
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    BLOCK_FLUSH,
    RANDOM
};

void initializeMutexAndCond() {
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cond_is_pending_request_empty, NULL);
    pthread_cond_init(&cond_is_woerker_threads_and_pending_requests_empty, NULL);
    pthread_cond_init(&cond_is_buffer_available, NULL);
}



// HW3: Parse the new arguments too
void getargs(int *port, int* threads_num, int* queue_size, enum SCHEDULER_ALGORITHM* schedalg, int argc, char *argv[])
{
    if (argc < ARGS_SIZE) {
        fprintf(stderr, "Usage: %s <portnum> <threads> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);

    if (strcmp(argv[4], "block") == 0) {
        *schedalg = BLOCK;
    } else if (strcmp(argv[4], "dt") == 0) {
        *schedalg = DROP_TAIL;
    } else if (strcmp(argv[4], "dh") == 0) {
        *schedalg = DROP_HEAD;
    } else if (strcmp(argv[4], "bf") == 0) {
        *schedalg = BLOCK_FLUSH;
    } else if (strcmp(argv[4], "random") == 0) {
        *schedalg = RANDOM;
    } else {
        assert(false);
    }
}


void* processRequest(void* args){
    int threadIndex = *((int *) args);

    while(1) {

        pthread_mutex_lock(&m);

        while(isEmpty(pendingRequestsQueue)) {
            pthread_cond_wait(&cond_is_pending_request_empty, &m);
        }

        Node* requestNode = dequeue(pendingRequestsQueue);
        Node* workThreadNode = enqueue(workerThreadsQueue, requestNode->connfd, requestNode->arrival);
        freeNode(requestNode);


        struct timeval handle;
        gettimeofday(&handle, NULL);

        struct timeval dispatch;
        timersub(&handle, &workThreadNode->arrival, &dispatch);
        pthread_mutex_unlock(&m);
        requestHandle(workThreadNode->connfd, workThreadNode->arrival, dispatch, threadsStats[threadIndex]);
        Close(workThreadNode->connfd);

        pthread_mutex_lock(&m);
        removeNode(workerThreadsQueue, workThreadNode);

        if (isEmpty(workerThreadsQueue) && isEmpty(pendingRequestsQueue)) {
            pthread_cond_signal(&cond_is_woerker_threads_and_pending_requests_empty);
        }

        pthread_cond_signal(&cond_is_buffer_available);
        pthread_mutex_unlock(&m);
    }
}


int main(int argc, char *argv[])
{
    int listenFd, connFd, port, clientLen, threadsNum, queueSize;
    enum SCHEDULER_ALGORITHM schedAlg;
    struct sockaddr_in clientAddr;
    initializeMutexAndCond();

    getargs(&port, &threadsNum, &queueSize, &schedAlg, argc, argv);
    assert(threadsNum > 0);
    assert(queueSize > 0);
    pendingRequestsQueue = createQueue(queueSize);
    workerThreadsQueue = createQueue(queueSize);
    pthread_t *threads = (pthread_t*)malloc(threadsNum * sizeof(pthread_t));


    for (int i = 0; i < threadsNum; i++) {
        int *threadIndex = malloc(sizeof(int));
        *threadIndex = i;
        if (pthread_create(&threads[i], NULL, processRequest, (void*) threadIndex) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    threadsStats = (thread_stats*) malloc(threadsNum * sizeof(thread_stats));
    for (int i = 0; i < threadsNum; i++) {
        threadsStats[i] = (thread_stats) malloc(sizeof(struct Thread_stats));
        threadsStats[i]->id = i;
        threadsStats[i]->stat_req = 0;
        threadsStats[i]->dynm_req = 0;
        threadsStats[i]->total_req = 0;
    }


    listenFd = Open_listenfd(port);
    while (1) {

        printQueue(workerThreadsQueue);
        clientLen = sizeof(clientAddr);
        connFd = Accept(listenFd, (SA *)&clientAddr, (socklen_t *) &clientLen);
        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        pthread_mutex_lock(&m);

        if (getSize(pendingRequestsQueue) + getSize(workerThreadsQueue) == queueSize){
            if (schedAlg == BLOCK){

                while(getSize(pendingRequestsQueue) + getSize(workerThreadsQueue) == queueSize){
                    assert(getSize(pendingRequestsQueue) + getSize(workerThreadsQueue) <= queueSize);
                    pthread_cond_wait(&cond_is_buffer_available, &m);
                }
            }
            else if (schedAlg == DROP_TAIL) {
                Close(connFd);
                pthread_mutex_unlock(&m);
                continue;
            }
            else if (schedAlg == DROP_HEAD) {
                if (isEmpty(pendingRequestsQueue)){
                    Close(connFd);
                    pthread_mutex_unlock(&m);
                    continue;
                }
                else {
                    Node* head = dequeue(pendingRequestsQueue);
                    Close(head->connfd);
                    freeNode(head);
                }
            }
            else if (schedAlg == BLOCK_FLUSH) {
                while (getSize(workerThreadsQueue) > 0){
                    pthread_cond_wait(&cond_is_woerker_threads_and_pending_requests_empty, &m);
                }
                Close(connFd);
                pthread_mutex_unlock(&m);
                continue;
            }
            else if (schedAlg == RANDOM) {
                int num_to_drop = (int)((getSize(pendingRequestsQueue) + 1) / 2);

                if (isEmpty(pendingRequestsQueue)){
                    assert(num_to_drop == 0);
                    Close(connFd);
                    pthread_mutex_unlock(&m);
                    continue;
                }
                assert(!isEmpty(pendingRequestsQueue));

                for (int i = 0; i < num_to_drop; i++) {
                    assert(!isEmpty(pendingRequestsQueue));
                    int element_index_to_drop = rand() % getSize(pendingRequestsQueue);
                    Node* node_to_drop = getNodeInIndex(pendingRequestsQueue, element_index_to_drop);
                    Close(node_to_drop->connfd);
                    removeNode(pendingRequestsQueue, node_to_drop);
                }

            }
        }

        enqueue(pendingRequestsQueue, connFd, arrival);
        pthread_cond_signal(&cond_is_pending_request_empty);
        pthread_mutex_unlock(&m);
    }
}
