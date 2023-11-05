#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>

#include "../mysql/sql_conn_pool.h"
#include "../timer/mytimer.h"

class HttpConn {
  public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

  public:
    HttpConn(){};
    ~HttpConn(){};
    void init(int sockfd, const sockaddr_in &addr, char *root,
              int conn_trig_mode, int close_log);
    void close_conn();
    void process();
    bool read();
    bool write();
    sockaddr_in *get_address() { return &m_address; }
    void init_mysql_result(SqlPool *sqlpool);
    int timer_flag;
    int io_finish;

  private:
    void init_http_status();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

  public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *m_mysql;
    int m_state; // 读为0, 写为1

  private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE];
    long m_read_idx;
    long m_checked_idx;
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    long m_content_length;
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        // 是否启用的POST
    char *m_string; // 存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;

    static locker m_lock;
    static map<string, string> m_users;
    int m_trig_mode;
    int m_close_log;
};

#endif