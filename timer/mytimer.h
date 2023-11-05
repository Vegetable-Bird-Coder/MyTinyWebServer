#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

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
}

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
    int setnonblocking(int fd);

    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int trig_mode);

    // 信号处理函数
    static void sig_handler(int sig);

    // 设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

  public:
    static int *m_pipefd;
    sort_timer_lst m_timer_lst;
    static int m_epollfd;
    int m_TIMESLOT;
};

#endif