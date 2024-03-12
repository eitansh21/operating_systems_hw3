//
// Created by shira on 01/03/2024.
//

#ifndef TASK_3_OS_QUEUE_H
#define TASK_3_OS_QUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#define ERROR -1
#define SUCCESS 0



struct Element{
    int data;
    struct Element* next;
    struct Element* prev;
    struct timeval arrival;};

struct Queue{
    int currentSize;
    int maxSize;
    struct Element* head_queue;
    struct Element* end_queue;
};

/*   TODO: CHECK IF NEEDED :
typedef struct infoOfThread{
    int id;
    int Qsize;
    int staticCounter;
    int dynamicCounter;
    int totalCounter;
}infoOfThread; */


struct Queue* createQueue(int maxSize);
//infoOfThread* createThread(int id, int Qsize);

//queue functions:
int isFull(struct Queue* q);
int getSizeOfQueue(struct Queue* q);
int pushToQueue(struct Queue* q, int data, struct timeval time);
int popFromQueue(struct Queue* q);
struct timeval getHeadArrival(struct Queue* q);

int freeQueueFunction(struct Queue* q);


extern struct Queue* waiting_queue;
//extern struct Queue* running_threads_queue;

#endif //TASK_3_OS_QUEUE_H
