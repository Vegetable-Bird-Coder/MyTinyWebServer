#include "./config/config.h"

int main(int argc, char *argv[]) {
    // 需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "root";
    string databasename = "qgydb";

    // 命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    // 初始化
    server.init(config.PORT, user, passwd, databasename, config.log_mode,
                config.opt_linger, config.trig_mode, config.sql_num,
                config.thread_num, config.close_log, config.actor_model);

    // 日志
    server.log_init();

    // 数据库
    server.sql_pool_init();

    // 线程池
    server.thread_pool_init();

    // 触发模式
    server.trig_mode_init();

    // 监听
    server.event_listen();

    // 运行
    server.event_loop();

    return 0;
}