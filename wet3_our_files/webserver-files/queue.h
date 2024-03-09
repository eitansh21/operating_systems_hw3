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
    struct timeval arrival_time;
} Node;


Node* createNode(int connfd, struct timeval arrivalTime);
void freeNode(Node* n);

typedef struct {
    int size;
    int maxSize;
    Node* head;
    Node* end_queue;
} Queue;


Queue* createQueue(int maxSize);
bool isFull(Queue* q);
bool isEmpty(Queue* q);
int getSize(Queue* q);
Node* enqueue(Queue* q, int connfd, struct timeval arrivalTime);
Node* dequeue(Queue* q);
void removeAndDeleteNode(Queue* q, Node* n);

void freeQueue(Queue* q);




#endif //WET3_OUR_FILES_QUEUE_H
