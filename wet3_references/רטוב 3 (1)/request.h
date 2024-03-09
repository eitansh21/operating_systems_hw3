#ifndef __REQUEST_H__

void requestHandle(int fd, struct timeval arrival, struct timeval handle_time, int *static_thread, int *dynamic_thread,
                   int *thread_total, int index_thread);

#endif
