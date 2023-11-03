#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <netinet/in.h>

class MyTimer;

struct ConnTimer {
    sockaddr_in address;
    int sockfd;
    MyTimer *timer;
};

class Utils {};

#endif