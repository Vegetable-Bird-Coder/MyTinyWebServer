#ifndef CONFIG_H
#define CONFIG_H

#include "../webserver/webserver.h"

using namespace std;

class Config {
  public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char *argv[]);

    // 端口号
    int port;

    // 日志写入方式
    int log_mode;

    // 触发组合模式
    int trig_mode;

    // listenfd触发模式
    int listen_trig_mode;

    // connfd触发模式
    int conn_trig_mode;

    // 优雅关闭链接
    int opt_linger;

    // 数据库连接池数量
    int sql_num;

    // 线程池内的线程数量
    int thread_num;

    // 是否关闭日志
    int close_log;

    // 并发模型选择
    int actor_model;
};

#endif