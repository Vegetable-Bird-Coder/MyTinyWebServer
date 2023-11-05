#include "sql_conn_pool.h"

SqlPool::SqlPool() {
    m_used_conn = 0;
    m_free_conn = 0;
}

SqlPool *SqlPool::get_instance() {
    static SqlPool sqlpool;
    return &sqlpool;
}

void SqlPool::init(string url, string username, string password,
                   string database_name, int port, int max_conn,
                   int close_log) {
    m_url = url;
    m_port = port;
    m_username = username;
    m_password = password;
    m_database_name = database_name;
    m_close_log = close_log;
    for (int i = 0; i < max_conn; i++) {
        MYSQL *sql_conn = nullptr;
        sql_conn = mysql_init(sql_conn);
        if (!sql_conn) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        sql_conn = mysql_real_connect(sql_conn, url.c_str(), username.c_str(),
                                      password.c_str(), database_name.c_str(),
                                      port, NULL, 0);

        if (!sql_conn) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        connpool.push_back(sql_conn);
        ++m_free_conn;
    }

    reserve = sem(m_free_conn);

    m_max_conn = m_free_conn;
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *SqlPool::get_connection() {
    MYSQL *con = NULL;

    if (0 == connpool.size())
        return NULL;

    reserve.wait();

    lock.lock();

    con = connpool.front();
    connpool.pop_front();

    --m_free_conn;
    ++m_used_conn;

    lock.unlock();
    return con;
}

// 释放当前使用的连接
bool SqlPool::release_connection(MYSQL *con) {
    if (NULL == con)
        return false;

    lock.lock();

    connpool.push_back(con);
    ++m_free_conn;
    --m_used_conn;

    lock.unlock();

    reserve.post();
    return true;
}

// 销毁数据库连接池
void SqlPool::destroy_pool() {

    lock.lock();
    if (connpool.size() > 0) {
        list<MYSQL *>::iterator it;
        for (it = connpool.begin(); it != connpool.end(); ++it) {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_used_conn = 0;
        m_free_conn = 0;
        connpool.clear();
    }

    lock.unlock();
}

SqlPool::~SqlPool() { destroy_pool(); }

SqlConnRAII::SqlConnRAII(MYSQL **sql_conn, SqlPool *sql_pool) {
    *sql_conn = sql_pool->get_connection();
    sql_conn_raii = *sql_conn;
    sql_pool_raii = sql_pool;
}

SqlConnRAII::~SqlConnRAII() {
    sql_pool_raii->release_connection(sql_conn_raii);
}