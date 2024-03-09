#include "queue.h"
//
// Created by shira on 01/03/2024.
//
/* TODO: CHECK IF NEEDED
infoOfThread* createThread(int id, int Qsize) {
    infoOfThread* newThread = (infoOfThread*)malloc(sizeof(infoOfThread));
    if (!newThread) {
        exit(1);
    }
    newThread->id = id;
    newThread->Qsize = Qsize;
    newThread->staticCounter = 0;
    newThread->dynamicCounter = 0;
    newThread->totalCounter = 0;

    return newThread;
}*/

bool isFull(struct Queue* q) {
    bool ans = q->currentSize == q->maxSize;
    return ans;
}

int getSizeOfQueue(struct Queue* q) {
    return q->currentSize;
}

int pushToQueue(struct Queue* q, int data, struct timeval time) {
    if (isFull(q)) {
        return ERROR;
    }
    struct Element* newElement = (struct Element*)malloc(sizeof(struct Element));
    if (!newElement) {
        return ERROR;  // Indicates failure
    }

    newElement->data = data;
    newElement->arrival = time;
    newElement->next = NULL;
    newElement->prev = NULL;

    if (q->currentSize == 0) {
        q->head_queue = newElement;
        q->end_queue = newElement;
    } else {
        newElement->prev = q->end_queue;
        q->end_queue->next = newElement;
        q->end_queue = newElement;
    }

    q->currentSize++;
    return SUCCESS;
}

// Function to pop data from the queue
int popFromQueue(struct Queue* q) {
    if (q->currentSize == 0) {
        return ERROR;
    }

    struct Element* temp = q->head_queue;
    int data_of_head = temp->data;

    if (getSizeOfQueue(q) > 1){ //needs at least 2 threads in the queue in order to manage that
        q->head_queue = temp->next;
        q->head_queue->prev = NULL;
    }
    else {
        q->end_queue = NULL;
        q->head_queue = NULL;
    }
    q->currentSize--;
    free(temp); //free the memory we have allocated

    return data_of_head;
}

// Function to create a new queue
struct Queue* createQueue(int maxSize) {
    struct Queue* newQueue = (struct Queue*)malloc(sizeof(struct Queue));
    if (!newQueue) {
        exit(EXIT_FAILURE);
    }

    newQueue->currentSize = 0;
    newQueue->maxSize = maxSize;
    newQueue->head_queue = NULL;
    newQueue->end_queue = NULL;

    return newQueue;
}

int freeQueueFunction(struct Queue* q) {
    if (q == NULL) {
        return ERROR;
    }

    struct Element* current = q->head_queue;
    while (current != NULL) {
        struct Element* next = current->next;
        free(current);
        current = next;
    }

    free(q);
    return SUCCESS;
}

struct timeval getHeadArrival(struct Queue* q){
    return q->head_queue->arrival;
}

/*
void runQueueTests() {
    struct Queue* myQueue = createQueue(5);

    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    // Test: Push elements into the queue
    assert(pushToQueue(myQueue, 10, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 20, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 30, currentTime) == SUCCESS);

    // Test: Check queue size
    assert(getSizeOfQueue(myQueue) == 3);

    // Test: Pop elements from the queue
    int popped1 = popFromQueue(myQueue);
    assert(popped1 == 10);

    int popped2 = popFromQueue(myQueue);
    assert(popped2 == 20);

    // Test: Check queue size after pops
    assert(getSizeOfQueue(myQueue) == 1);

    // Test: Push more elements into the queue
    assert(pushToQueue(myQueue, 40, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 50, currentTime) == SUCCESS);

    // Test: Check queue size after pushes
    assert(getSizeOfQueue(myQueue) == 3);

    // Test: Pop remaining elements
    int popped3 = popFromQueue(myQueue);
    int popped4 = popFromQueue(myQueue);

    // Test: Check queue size after pops
    assert(getSizeOfQueue(myQueue) == 1);
    int popped_5 = popFromQueue(myQueue);
    assert(popped_5 == 50);
    // Test: Attempt to pop from an empty queue
    assert(popFromQueue(myQueue) == ERROR);

    // Test: Push more elements to reach the maximum size
    assert(pushToQueue(myQueue, 60, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 70, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 80, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 90, currentTime) == SUCCESS);
    assert(pushToQueue(myQueue, 100, currentTime) == SUCCESS);

    // Test: Attempt to push into a full queue
    assert(pushToQueue(myQueue, 110, currentTime) == ERROR);

    // Test: Check queue size after reaching the maximum size
    assert(getSizeOfQueue(myQueue) == 5);

    // Test: Pop all elements from the queue
    int popped5 = popFromQueue(myQueue);
    int popped6 = popFromQueue(myQueue);
    int popped7 = popFromQueue(myQueue);
    int popped8 = popFromQueue(myQueue);
    int popped9 = popFromQueue(myQueue);

    // Test: Check queue size after pops
    assert(getSizeOfQueue(myQueue) == 0);

    // Test: Attempt to pop from an empty queue
    assert(popFromQueue(myQueue) == ERROR);

    // Cleanup: Free the memory allocated for the queue
    assert(destroyQueue(myQueue) == SUCCESS);

    printf("All tests passed successfully.\n");
}
*/


/**
int main() {
    runQueueTests();
    infoOfThread* myThread = createThread(1);
    free(myThread);
    return 0;
}*/