#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
class MyTimer;

struct ConnTimer {
    sockaddr_in address;
    int sockfd;
    MyTimer *timer;
};

class MyTimer {
  public:
    MyTimer() : prev(nullptr), next(nullptr) {}

  public:
    time_t expire;

    void (*cb_func)(ConnTimer *);
    ConnTimer *conn_timer;
    MyTimer *prev;
    MyTimer *next;
};

class sort_timer_lst {
  public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(MyTimer *timer);
    void adjust_timer(MyTimer *timer);
    void del_timer(MyTimer *timer);
    void tick();

  private:
    void add_timer(MyTimer *timer, MyTimer *lst_head);

    MyTimer *head;
    MyTimer *tail;
};

class Utils {
  public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    // 对文件描述符设置非阻塞
    static int setnonblocking(int fd);

    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    static void addfd(int epollfd, int fd, bool one_shot, int trig_mode);

    // 信号处理函数
    static void sig_handler(int sig);

    // 从内核时间表删除描述符
    static void removefd(int epollfd, int fd) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
        close(fd);
    }

    // 将事件重置为EPOLLONESHOT
    static void modfd(int epollfd, int fd, int ev, int trig_mode) {
        epoll_event event;
        event.data.fd = fd;

        if (1 == trig_mode)
            event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        else
            event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
    }

    // 设置信号函数
    static void addsig(int sig, void(handler)(int), bool restart = true);

    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    static void show_error(int connfd, const char *info);

  public:
    static int *m_pipefd;
    sort_timer_lst m_timer_lst;
    static int m_epollfd;
    int m_TIMESLOT;
};

#endif