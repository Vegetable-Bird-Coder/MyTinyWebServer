#ifndef WBESERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"
#include "../timer/mytimer.h"

const int MAX_FD = 65536;           // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5;             // 最小超时单位

class WebServer {
  public:
    WebServer();
    ~WebServer();

    void init(int port, string username, string password, string database_name,
              int log_mode, int opt_linger, int trig_mode, int sql_num,
              int thread_num, int close_log, int actor_model);
    void thread_pool_init();
    void sql_pool_init();
    void log_init();
    void trig_mode_init();
    void event_listen();
    void event_loop();
    void timer_init();
    // 连接期间有相应，更新计时器
    void timer_update();
    // conn长时间未响应，需要断开连接
    void timer_delete();
    void deal_connection();
    void deal_signal();
    void deal_read();
    void deal_write();

  public:
    int m_port;
    char *m_root; // 根目录
    int m_log_mode;
    int m_close_log;
    int m_actor_model;

    int m_pipefd[2]; // 用于统一事件源
    int m_epollfd;
    HttpConn *users;

    SqlPool *m_sql_pool;
    int m_sql_num;
    string m_username;
    string m_password;
    string m_database_name;

    ThreadPool<HttpConn> *m_thread_pool;
    int m_thread_num;

    epoll_event events[MAX_EVENT_NUMBER];
    int m_listenfd;
    int m_opt_linger;
    int m_trig_mode;
    int m_listen_trig_mode;
    int m_conn_trig_mode;

    ConnTimer *users_timer;
    Utils utils;
}

#endif