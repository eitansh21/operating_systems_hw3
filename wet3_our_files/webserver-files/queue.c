#include "queue.h"

Node* createNode(int connFd, struct timeval arrival){
    Node* res = (Node*)malloc(sizeof(Node));
    res->connFd = connFd;
    res->arrival = arrival;
    return res;
}

int isEmpty(Queue* q){
    return q->size == 0;
}

int getSize(Queue *q) {
    return q->size;
}

Node* enqueue(Queue* q, int connFd, struct timeval arrival){

    Node* newNode = createNode(connFd, arrival);

    if (q->size == 0) {
        q->head = newNode;
        q->end = newNode;
    } else {
        newNode->prev = q->end;
        q->end->next = newNode;
        q->end = newNode;
    }

    q->size++;
    return newNode;
}


Node* dequeue(Queue* q) {

    Node* headNode = q->head;

    if (getSize(q) > 1) {
        q->head = headNode->next;
        q->head->prev = NULL;
    }
    else {
        q->end = NULL;
        q->head = NULL;
    }
    q->size--;

    return headNode;
}

Queue* createQueue(){
    Queue* res = (Queue*)malloc(sizeof(Queue));
    res->head = NULL;
    res->end = NULL;
    res->size = 0;
    return res;
}


void removeNode(Queue* q, Node* n) {
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
            if (curr == q->end) {
                q->end = curr->prev;
            }
            q->size--;
            free(curr);
            return;
        }
        curr = curr->next;
    }
}

Node* getNodeInIndex(Queue* q, int index) {
    Node* curr = q->head;
    for (int i = 0; i < index; i++) {
        curr = curr->next;
    }
    return curr;
}
