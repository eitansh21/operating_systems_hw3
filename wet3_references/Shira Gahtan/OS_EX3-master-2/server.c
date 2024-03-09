#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#define MAX_SCHEDALG_SIZE 7
pthread_mutex_t mutex_master_and_worker = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t block_schedalg_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t both_queues_empty_cond = PTHREAD_COND_INITIALIZER;
struct Queue* Requests_to_handle_queue;

static int threadesInProgress = 0;

#define NOT_TO_CONTINUE 1
#define TO_CONTINUE 0


enum schedalg_t {
    BLOCK,
    DROP_TAIL,
    DROP_HEAD,
    BLOCK_FLUSH,
    DROP_RANDOM
};


//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//
// HW3: Parse the new arguments too
void getargs(int *port,int *threads,int* queue_size,enum schedalg_t schedalg, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s [portnum] [threads] [queue_size] [schedalg]\n", argv[0]);
	exit(1);
    }
    if (strcmp(argv[4], "block") == 0) {
        schedalg = BLOCK;
    } else if (strcmp(argv[4], "dt") == 0) {
        schedalg = DROP_TAIL;
    } else if (strcmp(argv[4], "dh") == 0) {
        schedalg = DROP_HEAD;
    } else if (strcmp(argv[4], "bf") == 0) {
        schedalg = BLOCK_FLUSH;
    } else if (strcmp(argv[4], "random") == 0) {
        schedalg = DROP_RANDOM;
    } else {
        printf(stderr, "Wrong schedalg\n");
        exit(1);
    }

    *port =     atoi(argv[1]);
    *threads=   atoi(argv[2]);
    *queue_size=atoi(argv[3]);
    //*schedalg=  atoi(argv[4]);


}

void *workerThread(void *arg){
    struct Tstats* threadStats;
    threadStats->id=(int)arg;
    while(1){
        pthread_mutex_lock(&mutex_master_and_worker);
        while (Requests_to_handle_queue->currentSize == 0) {
            // Wait on condition variable
            pthread_cond_wait(&queue_not_empty, &mutex_master_and_worker);
        }
        //TODO:: add support for statistics
        struct timeval arrival_time=getHeadArrival(Requests_to_handle_queue);
        int connfd= popFromQueue(Requests_to_handle_queue);
        threadesInProgress++;
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        struct timeval dispatch_time;
        timersub(&current_time, &arrival_time,&dispatch_time);
        struct timeStats* timeStats;
        timeStats->arrival=arrival_time;
        timeStats->dispatch=dispatch_time;
        //TODO: check if needed : if (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) { }
        pthread_cond_signal(&block_schedalg_cond);

        pthread_mutex_unlock(&mutex_master_and_worker);
        requestHandle(connfd,timeStats,threadStats); //to send tStats to requestHandle
        threadesInProgress--;
        if ((Requests_to_handle_queue->currentSize == 0) && (threadesInProgress == 0) ){
            pthread_cond_signal(&both_queues_empty_cond); // for block_flush sched algorithm
        }
        close(connfd);

        //TODO:: add support for overloading handling

    }
}

void check_block_schedalg(int Qsize){
    if (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) {
        while (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) {
            pthread_cond_wait(&block_schedalg_cond, &mutex_master_and_worker);
        }
    }
}


void check_drop_tail_schedalg(int Qsize, int connfd){
    if (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) {
        close(connfd);
    }
}

int check_drop_head_schedalg(int Qsize, int connfd) {
    if (Requests_to_handle_queue->currentSize + threadesInProgress == Qsize) {
        if (Requests_to_handle_queue->currentSize > 0 ){
            int fd_of_popped = popFromQueue(Requests_to_handle_queue);
            close(fd_of_popped);
            return NOT_TO_CONTINUE;
        }
        else { // not suppose to happen
            close(connfd);
            return TO_CONTINUE;
        }
    }
    return NOT_TO_CONTINUE;
}

int check_block_flush_schedalg(int Qsize, int connfd) {
    if (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) {
        while (Requests_to_handle_queue->currentSize + threadesInProgress != 0) {
            pthread_cond_wait(&both_queues_empty_cond, &mutex_master_and_worker);
        }
        Close(connfd);
        return TO_CONTINUE;
    }
    return NOT_TO_CONTINUE;
}


void pop_from_queue_in_index(int element_index_to_drop){
    struct Element *current_element = Requests_to_handle_queue->head_queue;
    for (int i = 0; i < element_index_to_drop; i++) {
        current_element = current_element->next;
    }
    //current_element = element to drop
    if (current_element->prev == NULL) { // current_element is the head
        if (current_element == Requests_to_handle_queue->end_queue){ // current_element = head = tail
            Requests_to_handle_queue->end_queue = Requests_to_handle_queue->head_queue = NULL;
        }
        else { // current_element is only the head
            Requests_to_handle_queue->head_queue = current_element->next;
            Requests_to_handle_queue->head_queue->prev = NULL;
        }
    }
    else if (current_element->next == NULL){ // current element is the tail
        Requests_to_handle_queue->end_queue = current_element->prev;
        Requests_to_handle_queue->end_queue->next = NULL;
    }
    else { //current_element is not head and not tail
        current_element->prev->next = current_element->next;
        current_element->next->prev = current_element->prev;
    }
    Requests_to_handle_queue->currentSize--;
    close(current_element->data);
    free(current_element);
}



int check_drop_random_schedalg(int Qsize) {
    if (Requests_to_handle_queue->currentSize + threadesInProgress >= Qsize) {
        if (Requests_to_handle_queue->currentSize == 0) {
            return TO_CONTINUE; // not suppose to happen, because num of threads < Qsize
        }
        int num_to_drop, element_index_to_drop;
        if ( (Requests_to_handle_queue->currentSize + threadesInProgress) %2){ // odd number
            num_to_drop = (Requests_to_handle_queue->currentSize + threadesInProgress + 1) / 2;
        }
        else { // even number
            num_to_drop = (Requests_to_handle_queue->currentSize + threadesInProgress) / 2;
        }
        while (num_to_drop > 0){
            element_index_to_drop = rand() % (Requests_to_handle_queue->currentSize);
            pop_from_queue_in_index(element_index_to_drop);
            num_to_drop--;
        }
        return NOT_TO_CONTINUE;
    }
    return NOT_TO_CONTINUE;
}

int main(int argc, char *argv[])
{
    enum schedalg_t sched_alg;
    int listenfd, connfd, port, clientlenAdd;
    struct sockaddr_in clientaddr;
    struct timeval current_time;
    int threadsNum,Qsize, maxSize;


    getargs(&port,&threadsNum,&Qsize,sched_alg,argc, argv);

    pthread_t *threads = malloc(threadsNum * sizeof(pthread_t));
    if (threads == NULL) {
        //perror("Failed to allocate memory for threads");
        exit(EXIT_FAILURE);
    }
    Requests_to_handle_queue = createQueue(Qsize);
    // Create worker threads
    for (int i = 0; i < threadsNum; i++) {
        if (pthread_create(&threads[i], NULL, workerThread, i)!=0) {
            //perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    listenfd = Open_listenfd(port);
    while (1) {
        pthread_mutex_lock(&mutex_master_and_worker);
        clientlenAdd = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlenAdd);
        gettimeofday(&current_time, NULL);
        if (sched_alg == BLOCK){
            check_block_schedalg(Qsize);
            // we are not continuing because the process should get to the waiting queue
        }
        else if (sched_alg == DROP_TAIL){
            check_drop_tail_schedalg(Qsize, connfd);
            pthread_mutex_unlock(&mutex_master_and_worker);
            continue;
        }
        else if (sched_alg == DROP_HEAD){
            if (check_drop_head_schedalg(Qsize, connfd) == TO_CONTINUE) {
                pthread_mutex_unlock(&mutex_master_and_worker);
                continue;
            }
        }
        else if (sched_alg == BLOCK_FLUSH){
            if ( check_block_flush_schedalg(Qsize, connfd) == TO_CONTINUE){
                pthread_mutex_unlock(&mutex_master_and_worker);
                continue; // we will not handle this request if the condition we check is true
            }
        }
        else if (sched_alg == DROP_RANDOM){
            if (check_drop_random_schedalg(Qsize) == TO_CONTINUE){
                pthread_mutex_unlock(&mutex_master_and_worker);
                continue; //this continue is not suppose to happen
            }
        }
        if (pushToQueue(Requests_to_handle_queue,connfd,current_time) == ERROR){
            perror("Failed to push to queue"); //maybe there's allocation failure
            exit(EXIT_FAILURE);
        }
        pthread_cond_signal(&queue_not_empty);
        pthread_mutex_unlock(&mutex_master_and_worker);
    }

}


    


 
