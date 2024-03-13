#ifndef TASK_3_OS_QUEUE_H
#define TASK_3_OS_QUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

typedef struct Node{
    int connFd;
    struct Node* next;
    struct Node* prev;
    struct timeval arrival;
} Node;

Node* createNode(int connFd, struct timeval arrival);

typedef struct Queue{
    int size;
    struct Node* head;
    struct Node* end;
} Queue;


Queue* createQueue();
int isEmpty(Queue* q);
int getSize(Queue* q);
Node* enqueue(Queue* q, int connFd, struct timeval arrival);
Node* dequeue(Queue* q);
void removeNode(Queue* q, Node* n);
Node* getNodeInIndex(Queue* q, int index);


#endif //TASK_3_OS_QUEUE_H
