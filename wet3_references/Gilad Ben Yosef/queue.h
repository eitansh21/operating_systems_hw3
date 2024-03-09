#ifndef HW3_QUEUE
#define HW3_QUEUE

#include "segel.h"
#include "request.h"

typedef int data_t;

typedef struct node{
    data_t data;
    struct node* next;
} NODE;

typedef struct{
    NODE* front;
    NODE* back;
    int size;
} QUEUE;

NODE* createNode(data_t data){
    NODE* res = malloc(sizeof(NODE));
    res->data = data;
    res->next = NULL;
    return res;
}

QUEUE* createQueue(){
    QUEUE* res = malloc(sizeof(QUEUE));
    res->front = NULL;
    res->back = NULL;
    res->size = 0;
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

void push_back(QUEUE* queue, data_t data){
    ++queue->size;
    
    if(queue->front == NULL){
        queue->front = createNode(data);
        queue->back = queue->front;
    }
    else{
        NODE* back = queue->back;
        back->next = createNode(data);
        queue->back = back->next;
    }
}

void push_front(QUEUE* queue, data_t data){
    ++queue->size;
    NODE* head = createNode(data);
    head->next = queue->front;
    queue->front = head;
    if(queue->back == NULL) queue->back = queue->front;
}

void pop_front(QUEUE* queue){
    --queue->size;
    NODE* head = queue->front;
    queue->front = head->next;
    free(head);
    if(queue->front == NULL) queue->back = NULL;
}

NODE* findNewBack(NODE* head){
    if(head->next == NULL) return NULL;
    while (head->next->next != NULL)
        head = head->next;
    return head;
}

void pop_back(QUEUE* queue){
    --queue->size;
    NODE* tail = queue->back;
    queue->back = findNewBack(queue->front);
    free(tail);
    if(queue->back == NULL) queue->front = NULL;
}

#endif //HW3_QUEUE