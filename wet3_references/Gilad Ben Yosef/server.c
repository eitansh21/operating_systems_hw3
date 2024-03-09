#include "segel.h"
#include "request.h"
// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

//-----------------------our queue implementation:------------------------------

typedef struct node{
    int data;
    struct timeval arrivalTime;
    struct node* next;
} NODE;

typedef struct{
    NODE* front;
    NODE* back;
    int size;
} QUEUE;

NODE* createNode(int data, struct timeval t){
    NODE* res = (NODE*)malloc(sizeof(NODE));
    res->data = data;
    res->arrivalTime = t;
    res->next = NULL;
    return res;
}

QUEUE* createQueue(){
    QUEUE* res = (QUEUE*)malloc(sizeof(QUEUE));
    res->front = NULL;
    res->back = NULL;
    res->size = 0;
    return res;
}

void destroyNode(NODE* n){
    if(n == NULL) return;
    destroyNode(n->next);
    free(n);
}

void destroyQueue(QUEUE* queue){
    destroyNode(queue->front);
    free(queue);
}


int isEmpty(QUEUE* q){
    return (q->size==0);
}

struct timeval getFrontTime(QUEUE* queue){
    return queue->front->arrivalTime;
}
/*
int full(QUEUE* queue){
    return (queue->maxSize == queue->size);
}*/
void push_back(QUEUE* queue, int data, struct timeval t){
    ++queue->size;
    NODE* toAdd = createNode(data,t);
    if(queue->front == NULL){
        queue->front = toAdd;
    }
    else{
        queue->back->next = toAdd;
    }
    queue->back = toAdd;
}
/*
void push_front(QUEUE* queue, int data, struct timeval t){
    ++queue->size;
    NODE* head = createNode(data,t);
    head->next = queue->front;
    queue->front = head;
    if(queue->back == NULL) queue->back = queue->front;
}*/

int pop_front(QUEUE* queue){
    if(isEmpty(queue))
        return -1;
    --queue->size;
    NODE* head = queue->front;
    int res = head->data;
    queue->front = head->next;
    free(head);
    if(queue->front == NULL) queue->back = NULL;
    return res;
}
/*
NODE* findNewBack(NODE* head){
    if(head->next == NULL) return NULL;
    while (head->next->next != NULL)
        head = head->next;
    return head;
}
int pop_back(QUEUE* queue){
    if(isEmpty(queue))
        return -1;
    --queue->size;
    NODE* tail = queue->back;
    queue->back = findNewBack(queue->front);
    int fd=tail->data;
    free(tail);
    if(queue->back == NULL) queue->front = NULL;
    return fd;
}*/
void pop_by_data(QUEUE* queue, int fd){
    if(isEmpty(queue)) return;
    if(queue->front->data == fd){
        pop_front(queue);
        return;
    }
    NODE* previous = queue->front;
    for(NODE* it = previous->next; it; previous = it, it = it->next){
        if(it->data == fd){
            previous->next = it->next;
            if(it == queue->back) queue->back = previous;
            free(it);
            --queue->size;
            return;
        }
    }
}
/*int getSize(QUEUE* queue){
    return queue->size;
}*/

int pop_by_index(QUEUE* queue, int index){
    NODE* it = queue->front;
    while(index){
        it = it->next;
        index--;
    }
    int res = it->data;
    pop_by_data(queue, res);
    return res;
}

void printQueue(QUEUE* q){
    for(NODE* n = q->front; n != NULL; n = n->next){
        printf("%d, ", n->data);
    }
}

//-------------------------------global variables:-----------------------------
QUEUE* waitingQueue = NULL;
QUEUE* workingQueue = NULL;
pthread_mutex_t m;
pthread_cond_t c;
pthread_cond_t bc;
int* threadCountArr;
int* threadDynamicArr;
int* threadStaticArr;

//-----------------------------handler functions:---------------------------------------------------------------

void block(int maxThreads, int connfd, struct timeval time){
    while(workingQueue->size + waitingQueue->size >= maxThreads)
        pthread_cond_wait(&bc,&m);
    push_back(waitingQueue, connfd, time);
    pthread_cond_signal(&c);
}

void drop_tail(int connfd){
    Close(connfd);
}
void drop_head(int connfd,  struct timeval time){
    push_back(waitingQueue, connfd, time);
    Close(pop_front(waitingQueue));
    pthread_cond_signal(&c);
}
void block_flush(int connfd){
    while(!isEmpty(workingQueue) && !isEmpty(waitingQueue))
        pthread_cond_wait(&bc,&m);
    close(connfd);
}
void dynamic(int maxDynamic,int connfd, int* maxSize){
    close(connfd);
    if(waitingQueue->size < maxDynamic)
        ++(*maxSize);
}
void randomPop(int connfd, struct timeval time){
    if(isEmpty(waitingQueue)){
        close(connfd);
        return;
    }
    int half = (waitingQueue->size + 1) / 2;
    for(int i = 0; i < half; ++i){
        close(pop_by_index(waitingQueue,rand() % waitingQueue->size));
    }
    push_back(waitingQueue, connfd, time);
    pthread_cond_signal(&c);
}
void handleMaxRequests(char* handler, int maxDynamicSize, int connfd, int maxThreads, struct timeval time, int* maxSize){
    if(strcmp(handler,"block") == 0)
        block(maxThreads, connfd, time);
    else if(strcmp(handler,"dt") == 0)
        drop_tail(connfd);
    else if(strcmp(handler,"dh") == 0)
        drop_head(connfd, time);
    else if(strcmp(handler,"bf") == 0)
        block_flush(connfd);
    else if(strcmp(handler,"dynamic") == 0)
        dynamic(maxDynamicSize,connfd, maxSize);
    else if(strcmp(handler,"random") == 0)
        randomPop(connfd, time);
}

//-----------------------------------------------------more:-------------------------------------------------

void getargs(int *port,int *numOfThreads,int *queueSize,char* overloadHandler,int* maxDynamicSize, int argc, char *argv[])
{
    if (argc < 5) {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    *port = atoi(argv[1]);
    *numOfThreads = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    strcpy(overloadHandler,argv[4]);
    if(strcmp(overloadHandler,"dynamic")==0)
        *maxDynamicSize = atoi(argv[5]);
    else
        *maxDynamicSize = *queueSize;
}

void* createThread(void* args){
    int threadID = *((int*)args);
    struct timeval arrivalTime, currTime;
    while(1){
        pthread_mutex_lock(&m);
        while(isEmpty(waitingQueue))
            pthread_cond_wait(&c,&m);
        gettimeofday(&currTime,NULL);
        arrivalTime = getFrontTime(waitingQueue);
        int fd = pop_front(waitingQueue);
        push_back(workingQueue,fd,arrivalTime);
        ++threadCountArr[threadID];
        pthread_mutex_unlock(&m);
        int res = requestHandle(fd, arrivalTime, currTime, threadID, threadCountArr, threadDynamicArr, threadStaticArr);
        pthread_mutex_lock(&m);
        if(res == 1) ++threadStaticArr[threadID];
        if(res == 2) ++threadDynamicArr[threadID];
        pop_by_data(workingQueue, fd);
        pthread_cond_signal(&bc);
        Close(fd);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}
//-----------------------------------------------------------main-------------------------------------------------//
int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numOfThreads, maxSize, maxDynamicSize;
    struct sockaddr_in clientaddr;
    char overloadHandler[8];
    getargs(&port,&numOfThreads,&maxSize,overloadHandler,&maxDynamicSize, argc, argv);

    waitingQueue = createQueue(maxSize);
    workingQueue = createQueue(numOfThreads);
    threadCountArr = (int*) calloc(numOfThreads,sizeof(int));
    threadDynamicArr = (int*) calloc(numOfThreads,sizeof(int));
    threadStaticArr = (int*) calloc(numOfThreads,sizeof(int));
    // 
    // HW3: Create some threads...
    //
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
    pthread_cond_init(&bc, NULL);
    pthread_t* threadsArr = malloc(numOfThreads * sizeof(pthread_t));
    for(int i=0; i < numOfThreads; i++){
        pthread_create(&threadsArr[i],NULL,createThread,&i);
    }
    listenfd = Open_listenfd(port);
    struct timeval time;
    while(1){
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        gettimeofday(&time,NULL);
        pthread_mutex_lock(&m);

        if(waitingQueue->size + workingQueue->size >= maxSize)
        {
            if(strcmp(overloadHandler,"block") == 0){
                while(workingQueue->size + waitingQueue->size >= maxSize)
                    pthread_cond_wait(&bc,&m);
                push_back(waitingQueue, connfd, time);
                pthread_cond_signal(&c);
            }
            else if(strcmp(overloadHandler,"dt") == 0){
                close(connfd);
            }
            else if(strcmp(overloadHandler,"dh") == 0){
                push_back(waitingQueue, connfd, time);
                Close(pop_front(waitingQueue));
                pthread_cond_signal(&c);
            }
            else if(strcmp(overloadHandler,"bf") == 0){
                while(!isEmpty(workingQueue) && !isEmpty(waitingQueue))
                    pthread_cond_wait(&bc,&m);
                close(connfd);
            }
            else if(strcmp(overloadHandler,"dynamic") == 0){
                close(connfd);
                if(waitingQueue->size < maxDynamicSize)
                    ++maxSize;
            }
            else if(strcmp(overloadHandler,"random") == 0){
                if(isEmpty(waitingQueue)){
                    close(connfd);
                }
                else{
                    int half = (waitingQueue->size + 1) / 2;
                    for(int i = 0; i < half; ++i){
                        close(pop_by_index(waitingQueue,rand() % waitingQueue->size));
                    }
                    push_back(waitingQueue, connfd, time);
                    pthread_cond_signal(&c);
                }
            }
        }
        else
        {
            push_back(waitingQueue,connfd,time);
            pthread_cond_signal(&c);
        }
        pthread_mutex_unlock(&m);
    }
    return 0;
}

    


 
