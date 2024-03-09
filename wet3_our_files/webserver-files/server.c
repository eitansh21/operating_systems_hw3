#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>

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
pthread_cond_t cond;

Queue* workerThreadsQueue;
Queue* pendingRequestsQueue;

enum SCHEDUA_ALGORITHM {
    BLOCK,
    DT,
    DH,
    BF,
    RANDOM
};

void intializeMutexAndCond() {
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cond, NULL);
}



// HW3: Parse the new arguments too
void getargs(int *port, int* threads_num, int* queue_size, enum SCHEDUA_ALGORITHM* schedalg, int argc, char *argv[])
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
        *schedalg = DT;
    } else if (strcmp(argv[4], "dh") == 0) {
        *schedalg = DH;
    } else if (strcmp(argv[4], "bf") == 0) {
        *schedalg = BF;
    } else if (strcmp(argv[4], "random") == 0) {
        *schedalg = RANDOM;
    } else {
//        printf(stderr, "Wrong schedalg\n");
        exit(1);
    }
}


void* processRequest(void* args){
    int threadIndex = *((int *) args);

    while(1) {
        pthread_mutex_lock(&m);
        while(isEmpty(pendingRequestsQueue)){
            pthread_cond_wait(&cond, &m);
        }

        Node* requestNode = dequeue(pendingRequestsQueue);
        Node* workThreadNode = enqueue(workerThreadsQueue, requestNode->connfd, requestNode->arrival_time);
        pthread_mutex_unlock(&m);

        struct timeval handleTime;
        gettimeofday(&handleTime, NULL);

        requestHandle(requestNode->connfd, requestNode->arrival_time, handleTime, threadIndex);
        freeNode(requestNode);
        pthread_mutex_lock(&m);
        removeAndDeleteNode(workerThreadsQueue, workThreadNode);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}



int main(int argc, char *argv[])
{
    int listenFd, connFd, port, clientLen, threadsNum, queueSize;
    enum SCHEDUA_ALGORITHM schedAlg;
    struct sockaddr_in clientaddr;
    intializeMutexAndCond();


    getargs(&port, &threadsNum, &queueSize, &schedAlg, argc, argv);
    assert(threadsNum > 0);
    assert(queueSize > 0);
    pendingRequestsQueue = createQueue(queueSize);
    workerThreadsQueue = createQueue(queueSize);
    pthread_t *threads = (pthread_t*)malloc(threadsNum * sizeof(pthread_t));

    if (threads == NULL) {
//        fprintf(stderr, "Error: malloc failed\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < threadsNum; i++) {
        if (pthread_create(&threads[i], NULL, processRequest, (void*)& i)!=0) {
            exit(EXIT_FAILURE);
        }
    }





    listenFd = Open_listenfd(port);
    while (1) {
        clientLen = sizeof(clientaddr);
        connFd = Accept(listenFd, (SA *)&clientaddr, (socklen_t *) &clientLen);
    pthread_mutex_lock(&m);
	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	//
    if (getSize(pendingRequestsQueue) + getSize(workerThreadsQueue) == queueSize){
        if (schedAlg == BLOCK){
            while(getSize(pendingRequestsQueue) + getSize(workerThreadsQueue) == queueSize){
                pthread_cond_wait(&cond, &m);
            }
        }
        else if (schedAlg == DT){
            Close(connFd);
            pthread_mutex_unlock(&m);
            continue;
//            pop_from_queue_in_index(0);
        }
        else if (schedAlg == DH){
            dequeue(pendingRequestsQueue);
//            pop_from_queue_in_index(getSize(pendingRequestsQueue) - 1);
        }
        else if (schedAlg == BF){
            int element_index_to_drop = queue_find(pendingRequestsQueue, connFd);
            pop_from_queue_in_index(element_index_to_drop);
        }
        else if (schedAlg == RANDOM){
            int num_to_drop = rand() % getSize(pendingRequestsQueue);
            while(num_to_drop > 0){
                int element_index_to_drop = rand() % getSize(pendingRequestsQueue);
                pop_from_queue_in_index(element_index_to_drop);
                num_to_drop--;
            }
            pthread_mutex_unlock(&m);
            continue;
        }
//        pthread_mutex_unlock(&m);
//        continue;
    }
    struct timeval arrival;
    gettimeofday(&arrival, NULL);
    enqueue(pendingRequestsQueue, connFd, arrival);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&m);


//	requestHandle(connFd);

//	Close(connFd);
    }

}


    


 
