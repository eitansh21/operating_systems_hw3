#include "queue.h"


Node* createNode(int connfd, struct timeval arrivalTime){
    assert(connfd >= 0);

    Node* res = (Node*)malloc(sizeof(Node));
    res->connfd = connfd;
    res->arrival_time = arrivalTime;
    return res;
}

void freeNode(Node* n){
    assert(n != NULL);
    free(n);
}


Queue* createQueue(int maxSize){
    assert(maxSize > 0);
    Queue* res = (Queue*)malloc(sizeof(Queue));
    res->head = NULL;
    res->end_queue = NULL;
    res->size = 0;
    res->maxSize = maxSize;
    return res;
}

bool isFull(Queue* q){
    assert(q->size >= 0);
    return q->size == q->maxSize;
}

bool isEmpty(Queue* q){
    assert(q->size >= 0);
    return q->size == 0;
}

int getSize(Queue* q){
    assert(q->size >= 0);
    return q->size;
}

Node* enqueue(Queue* q, int connfd, struct timeval arrivalTime){
    assert(!isFull(q));

    Node* newNode = createNode(connfd, arrivalTime);

    if (q->size == 0) {
        q->head = newNode;
        q->end_queue = newNode;
    } else {
        newNode->prev = q->end_queue;
        q->end_queue->next = newNode;
        q->end_queue = newNode;
    }

    q->size++;
    return newNode;
}

Node* dequeue(Queue* q){
    assert(!isEmpty(q));

    Node* headNode = q->head;

    if (getSize(q) > 1) {
        q->head = headNode->next;
        q->head->prev = NULL;
    }
    else {
        q->end_queue = NULL;
        q->head = NULL;
    }
    q->size--;

    return headNode;
}

void removeAndDeleteNode(Queue* q, Node* n) {
    Node* curr = q->head;
    while (curr != NULL) {
        if (curr == n) {
            if (curr->prev != NULL) {
                curr->prev->next = curr->next;
            }
            if (curr->next != NULL) {
                curr->next->prev = curr->prev;
            }
            if (curr == q->head) {
                q->head = curr->next;
            }
            if (curr == q->end_queue) {
                q->end_queue = curr->prev;
            }
            q->size--;
            freeNode(curr);
            return;
        }
        curr = curr->next;
    }
}
