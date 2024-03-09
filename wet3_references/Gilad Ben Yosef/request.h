#ifndef __REQUEST_H__

int requestHandle(int fd, struct timeval arrivalTime, struct timeval currTime, int threadID, int* threadCountArr,
                    int* threadDynamicArr, int* threadStaticArr);

#endif