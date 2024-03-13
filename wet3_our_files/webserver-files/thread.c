#include "thread.h"
#include <stdlib.h>

thread_stats threadStatsCreate(int id) {
    thread_stats stats = malloc(sizeof(thread_stats));

    stats->id = id;
    stats->stat_req = 0;
    stats->dynm_req = 0;
    stats->total_req = 0;
    return stats;
}
