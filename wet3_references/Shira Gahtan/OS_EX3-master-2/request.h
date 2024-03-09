#ifndef __REQUEST_H__
#include "queue.h"

struct timeStats{
    struct timeval arrival;
    struct timeval dispatch;
};


typedef struct Tstats{
    int id;
    int stat_req;
    int dynm_req;
    int total_req;
};


void requestHandle(int fd,struct timeStats* tStats, struct Tstats* threadStats);

void requestPrintStats(struct timeStats* tStats, struct Tstats* threadStats);



#endif
