#include "webserver.h"

WebServer::WebServer() {
    users = new HttpConn[MAX_FD];

    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);

    users_timer = new ConnTimer[MAX_FD];
}

WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]);
    close(m_pipefd[1]);
    delete[] users;
    delete[] users_timer;
    delete[] m_thread_pool;
}

void WebServer::init(int port, string username, string password,
                     string database_name, int log_mode, int opt_linger,
                     int trig_mode, int sql_num, int thread_num,
                     int close_log, int actor_model) {
    m_port = port;
    m_username = username;
    m_password = password;
    m_database_name = database_name;
    m_log_mode = log_mode;
    m_opt_linger = opt_linger;
    m_trig_mode = trig_mode;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_close_log = close_log;
    m_actor_model = actor_model;
}

void WebServer::trig_mode_init() {
    if (m_trig_mode == 0) { // LT + LT
        m_listen_trig_mode = 0;
        m_conn_trig_mode = 0;
    } else if (m_trig_mode == 1) { // LT + ET
        m_listen_trig_mode = 0;
        m_conn_trig_mode = 1;
    } else if (m_trig_mode == 2) { // ET + LT
        m_listen_trig_mode = 1;
        m_conn_trig_mode = 0;
    } else if (m_trig_mode == 3) { // ET + ET
        m_listen_trig_mode = 1;
        m_conn_trig_mode = 1;
    }
}

void WebServer::log_init() {
    if (m_close_log == 0) {
        if (m_log_mode == 1)
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
}

void WebServer::sql_pool_init() {
    m_sql_pool = SqlPool::get_instance();
    m_sql_pool->init("localhost", m_username, m_password, m_database_name, 3306, m_sql_num, m_close_log);

    users->init_mysql_result(m_sql_pool);
}
