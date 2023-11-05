#ifndef SQL_CONN_POLL_H
#define SQL_CONN_POLL_H

#include <list>
#include <mysql/mysql.h>
#include <string>

#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class SqlPool {
  public:
    MYSQL *get_connection();
    bool release_connection(MYSQL *conn);
    void destroy_pool();

    static SqlPool *get_instance();

    void init(string url, string username, string password,
              string database_name, int port, int max_conn, int close_log);

  private:
    SqlPool();
    ~SqlPool();

    int m_max_conn;
    int m_used_conn;
    int m_free_conn;
    locker lock;
    list<MYSQL *> connpool;
    sem reserve;

  public:
    string m_url;
    string m_port;
    string m_username;
    string m_password;
    string m_database_name;
    int m_close_log;
};

class SqlConnRAII {
  public:
    SqlConnRAII(MYSQL **sql_conn, SqlPool *sqlpool);
    ~SqlConnRAII();

  private:
    MYSQL *sql_conn_raii;
    SqlPool *sql_pool_raii;
};

#endif