#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#include "thread.h"

#define ARGS_SIZE 5

pthread_mutex_t m;
pthread_cond_t cond_is_pending_request_empty;
pthread_cond_t cond_is_buffer_available;
pthread_cond_t cond_is_worker_threads_and_pending_requests_empty;
Queue* pendingRequestsQueue;
static int threadsInUse = 0;
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
    pthread_cond_init(&cond_is_worker_threads_and_pending_requests_empty, NULL);
    pthread_cond_init(&cond_is_buffer_available, NULL);
}

void getargs(int *port,int *threads_num,int* queue_size,enum SCHEDULER_ALGORITHM *schedalg, int argc, char *argv[])
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
        exit(1);
    }
}

void *processRequest(void *arg){
    int threadIndex = *((int *) arg);

    while(1){
        pthread_mutex_lock(&m);
        while(isEmpty(pendingRequestsQueue)) {
            pthread_cond_wait(&cond_is_pending_request_empty, &m);
        }
        Node* requestNode = dequeue(pendingRequestsQueue);

        struct timeval handle;
        gettimeofday(&handle, NULL);
        struct timeval dispatch_time;
        timersub(&handle, &requestNode->arrival, &dispatch_time);
        threadsInUse++;
        pthread_mutex_unlock(&m);
        requestHandle(requestNode->connFd, requestNode->arrival, dispatch_time, threadsStats[threadIndex]);
        Close(requestNode->connFd);
        free(requestNode);
        pthread_mutex_lock(&m);
        pthread_cond_signal(&cond_is_buffer_available);
        threadsInUse--;

        if (isEmpty(pendingRequestsQueue) && threadsInUse == 0){
            pthread_cond_signal(&cond_is_worker_threads_and_pending_requests_empty); // for block_flush sched algorithm
        }
        pthread_mutex_unlock(&m);
    }
}

int main(int argc, char *argv[])
{
    int listenFd, connFd, port, clientLenAdd,threadsNum, queueSize;
    enum SCHEDULER_ALGORITHM schedAlg;
    struct sockaddr_in clientAddr;
    initializeMutexAndCond();

    getargs(&port, &threadsNum, &queueSize, &schedAlg, argc, argv);
    pendingRequestsQueue = createQueue();

    pthread_t *threads = malloc(threadsNum * sizeof(pthread_t));
    threadsStats = malloc(threadsNum * sizeof(thread_stats));

    for (int i = 0; i < threadsNum; i++) {
        int* threadIndex = malloc(sizeof(int));
        *threadIndex = i;
        pthread_create(&threads[i], NULL, processRequest, (void*)threadIndex);
        threadsStats[i] = threadStatsCreate(i);
    }

    listenFd = Open_listenfd(port);

    while (1) {
        clientLenAdd = sizeof(clientAddr);
        connFd = Accept(listenFd, (SA *)&clientAddr, (socklen_t *) &clientLenAdd);
        struct timeval arrival;
        gettimeofday(&arrival, NULL);
        pthread_mutex_lock(&m);

        if (getSize(pendingRequestsQueue) + threadsInUse == queueSize) {
            if (schedAlg == BLOCK) {
                while (getSize(pendingRequestsQueue) + threadsInUse == queueSize) {
                    pthread_cond_wait(&cond_is_buffer_available, &m);
                }
            } else if (schedAlg == DROP_TAIL) {
                Close(connFd);
                pthread_mutex_unlock(&m);
                continue;
            } else if (schedAlg == DROP_HEAD) {
                if (!isEmpty(pendingRequestsQueue)) {
                    Node* head = dequeue(pendingRequestsQueue);
                    Close(head->connFd);
                } else {
                    Close(connFd);
                    pthread_mutex_unlock(&m);
                    continue;
                }
            } else if (schedAlg == BLOCK_FLUSH) {
                while (getSize(pendingRequestsQueue) + threadsInUse > 0) {
                    pthread_cond_wait(&cond_is_worker_threads_and_pending_requests_empty, &m);
                }
                Close(connFd);
                pthread_mutex_unlock(&m);
                continue;
            } else if (schedAlg == RANDOM) {

                if (isEmpty(pendingRequestsQueue)) {
                    Close(connFd);
                    pthread_mutex_unlock(&m);
                    continue;
                }

                int numToDrop = (int) ((getSize(pendingRequestsQueue) + 1) / 2);

                for (int i = 0; i < numToDrop; i++) {
                    int element_index_to_drop = rand() % getSize(pendingRequestsQueue);
                    Node* node_to_drop = getNodeInIndex(pendingRequestsQueue, element_index_to_drop);
                    Close(node_to_drop->connFd);
                    removeNode(pendingRequestsQueue, node_to_drop);
                }
            }
        }
        enqueue(pendingRequestsQueue, connFd, arrival);
        pthread_cond_signal(&cond_is_pending_request_empty);
        pthread_mutex_unlock(&m);
    }
}
