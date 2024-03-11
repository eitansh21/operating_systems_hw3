#ifndef WET3_OUR_FILES_QUEUE_H
#define WET3_OUR_FILES_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>

#define ERROR_CODE -1
#define SUCCESS_CODE 0

typedef struct Node{
    int connfd;
    struct Node* next;
    struct Node* prev;
    struct timeval arrival;
} Node;


Node* createNode(int connfd, struct timeval arrival);
void freeNode(Node* n);

typedef struct Queue {
    int size;
    int maxSize;
    Node* head;
    Node* end_queue;
} Queue;


Queue* createQueue(int maxSize);
bool isFull(Queue* q);
bool isEmpty(Queue* q);
int getSize(Queue* q);
Node* enqueue(Queue* q, int connFd, struct timeval arrival);
Node* dequeue(Queue* q);
void removeNode(Queue* q, Node* n);
Node* getNodeInIndex(Queue* q, int index);

void freeQueue(Queue* q);
void printQueue(Queue* q);




#endif //WET3_OUR_FILES_QUEUE_H
