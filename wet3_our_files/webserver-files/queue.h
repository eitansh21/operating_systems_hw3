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
    int data;
    struct Node* next;
    struct Node* prev;
    struct timeval arrival;
} Node;


Node* createNode(int data, struct timeval time);
void freeNode(Node* n);

typedef struct {
    int currentSize;
    int maxSize;
    Node* head_queue;
    Node* end_queue;
} Queue;


Queue* createQueue(int maxSize);
bool isFull(Queue* q);
bool isEmpty(Queue* q);
int getSize(Queue* q);
int enqueue(Queue* q, int data, struct timeval time);
int dequeue(Queue* q);
struct timeval getHeadArrival(Queue* q);
void freeQueue(Queue* q);




#endif //WET3_OUR_FILES_QUEUE_H
