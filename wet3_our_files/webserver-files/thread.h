#ifndef WET3_BACKUP_THREAD_H
#define WET3_BACKUP_THREAD_H

typedef struct Thread_stats{
    int id;
    int stat_req;
    int dynm_req;
    int total_req;
} *thread_stats;

thread_stats threadStatsCreate(int threadsNum);

#endif //WET3_BACKUP_THREAD_H
