#include "queue.h"



Node* createNode(int data, struct timeval time){
    Node* res = malloc(sizeof(Node));
    res->data = data;
    res->arrival = time;
    return res;
}

void freeNode(Node* n){
    free(n);
}




Queue* createQueue(int maxSize){
    Queue* res = malloc(sizeof(Queue));
    res->head_queue = NULL;
    res->end_queue = NULL;
    res->currentSize = 0;
    res->maxSize = maxSize;
    return res;
}

bool isFull(Queue* q){
    return q->currentSize == q->maxSize;
}

bool isEmpty(Queue* q){
    return q->currentSize == 0;
}

int getSize(Queue* q){
    return q->currentSize;
}

int enqueue(Queue* q, int data, struct timeval time){
    if (isFull(q)) {
        return ERROR_CODE;
    }
    Node* newNode = createNode(data, time);
    //TODO: check if we need to free the memory of newNode if we fail to allocate memory
    if (!newNode) {
        return ERROR_CODE;  // Indicates failure
    }

    if (q->currentSize == 0) {
        q->head_queue = newNode;
        q->end_queue = newNode;
    } else {
        newNode->prev = q->end_queue;
        q->end_queue->next = newNode;
        q->end_queue = newNode;
    }

    q->currentSize++;
    return SUCCESS_CODE;
}

int dequeue(Queue* q){
    if (q->currentSize == 0) {
        return ERROR_CODE;
    }

    Node* temp = q->head_queue;
    int data_of_head = temp->data;

    if (getSize(q) > 1){ //needs at least 2 threads in the queue in order to manage that
        q->head_queue = temp->next;
        q->head_queue->prev = NULL;
    }
    else {
        q->end_queue = NULL;
        q->head_queue = NULL;
    }
    q->currentSize--;
    freeNode(temp); //free the memory we have allocated

    return data_of_head;
}


