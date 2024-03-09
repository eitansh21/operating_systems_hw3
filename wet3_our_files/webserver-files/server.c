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
    int thread_index =((int *)args)[0];

    while(1){
        pthread_mutex_lock(&m);
        while(isEmpty(pendingRequestsQueue)){
            pthread_cond_wait(&cond, &m);
        }
        // handle
        struct timeval arrival = queue_head_arrival_time(pendingRequestsQueue);
        int fd = dequeue(pendingRequestsQueue);
        enqueue(workerThreadsQueue, fd, arrival);
        pthread_mutex_unlock(&m);

        struct timeval handle_time;
        gettimeofday(&handle_time,NULL);

        requestHandle(fd, arrival, handle_time, thread_index);

        pthread_mutex_lock(&m);
        dequeue_index(workerThreadsQueue, queue_find(workerThreadsQueue,fd));
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}




int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threadsNum, queueSize;
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





    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
