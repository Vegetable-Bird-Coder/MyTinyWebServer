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
                     int trig_mode, int sql_num, int thread_num, int close_log,
                     int actor_model) {
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
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000,
                                      800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000,
                                      0);
    }
}

void WebServer::sql_pool_init() {
    m_sql_pool = SqlPool::get_instance();
    m_sql_pool->init("localhost", m_username, m_password, m_database_name, 3306,
                     m_sql_num, m_close_log);

    users->init_mysql_result(m_sql_pool);
}

void WebServer::thread_pool_init() {
    m_thread_pool =
        new ThreadPool<HttpConn>(m_actor_model, m_sql_pool, m_thread_num);
}

void WebServer::event_listen() {
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listenfd < 0)
        throw std::exception();

    if (m_opt_linger == 0) {
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (m_opt_linger == 1) {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    if (ret < 0)
        throw std::exception();
    ret = listen(m_listenfd, 5);
    if (ret < 0)
        throw std::exception();

    utils.init(TIMESLOT);

    m_epollfd = epoll_create(5);
    if (m_epollfd == -1)
        throw std::exception();

    utils.addfd(m_epollfd, m_listenfd, false, m_listen_trig_mode);
    HttpConn::m_epollfd = m_epollfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    if (ret == -1)
        throw std::exception();
    Utils::setnonblocking(
        m_pipefd[1]); // 计时器到期之类任务没有那么紧急，因此读可以是非阻塞的
    Utils::addfd(m_epollfd, m_pipefd[0], false, 0);

    Utils::addsig(SIGPIPE, SIG_IGN);
    Utils::addsig(SIGALRM, utils.sig_handler, false);
    Utils::addsig(SIGALRM, utils.sig_handler, false);

    alarm(TIMESLOT);

    Utils::m_pipefd = m_pipefd;
    Utils::m_epollfd = m_epollfd;
}

void WebServer::timer_init(int connfd, struct sockaddr_in client_address) {
    users[connfd].init(connfd, client_address, m_root, m_conn_trig_mode,
                       m_close_log);

    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    MyTimer *timer = new MyTimer;
    timer->conn_timer = &users_timer[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(nullptr);
    timer->expire = cur + 3 * TIMESLOT;
    users_timer[connfd].timer = timer;
    utils.m_timer_lst.add_timer(timer);
}

void WebServer::timer_update(MyTimer *timer) {
    time_t cur = time(nullptr);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
    LOG_INFO("%s", "adjust timer once");
}

void WebServer::timer_delete(MyTimer *timer, int sockfd) {
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
        utils.m_timer_lst.del_timer(timer);
    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

bool WebServer::deal_connection() {
    struct sockaddr_in client_address;
    socklen_t client_addr_len = sizeof(client_address);
    if (m_listen_trig_mode == 0) { // LT
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address,
                            &client_addr_len);
        if (connfd < 0) {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (HttpConn::m_user_count >= MAX_FD) {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer_init(connfd, client_address);
    } else {
        while (1) {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address,
                                &client_addr_len);
            if (connfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return true;
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (HttpConn::m_user_count >= MAX_FD) {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                close(connfd);
                break;
            }
            timer_init(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool WebServer::deal_signal(bool &timeout, bool &stop_server) {
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1) {
        return false;
    } else if (ret == 0) {
        return false;
    } else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGALRM: {
                timeout = true;
                break;
            }
            case SIGTERM: {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::deal_read(int sockfd) {
    MyTimer *timer = users_timer[sockfd].timer;

    // reactor
    if (1 == m_actor_model) {
        if (timer) {
            timer_update(timer);
        }

        // 若监测到读事件，将该事件放入请求队列
        m_thread_pool->append(users + sockfd, 0);

        while (true) {
            if (users[sockfd].io_finish) {
                if (users[sockfd].timer_flag) {
                    timer_delete(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].io_finish = 0;
                break;
            }
        }
    } else {
        // proactor
        if (users[sockfd].read()) {
            LOG_INFO("deal with the client(%s)",
                     inet_ntoa(users[sockfd].get_address()->sin_addr));

            // 若监测到读事件，将该事件放入请求队列
            m_thread_pool->append(users + sockfd);

            if (timer) {
                timer_update(timer);
            }
        } else {
            timer_delete(timer, sockfd);
        }
    }
}

void WebServer::deal_write(int sockfd) {
    MyTimer *timer = users_timer[sockfd].timer;
    // reactor
    if (m_actor_model == 1) {
        if (timer) {
            timer_update(timer);
        }

        m_thread_pool->append(users + sockfd, 1);

        while (true) {
            if (users[sockfd].io_finish) {
                if (users[sockfd].timer_flag) {
                    timer_delete(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].io_finish = 0;
                break;
            }
        }
    } else {
        // proactor
        if (users[sockfd].write()) {
            LOG_INFO("send data to the client(%s)",
                     inet_ntoa(users[sockfd].get_address()->sin_addr));

            if (timer) {
                timer_update(timer);
            }
        } else {
            timer_delete(timer, sockfd);
        }
    }
}

void WebServer::event_loop() {
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server) {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == m_listenfd) {
                bool flag = deal_connection();
                if (false == flag)
                    continue;
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 服务器端关闭连接，移除对应的定时器
                MyTimer *timer = users_timer[sockfd].timer;
                timer_delete(timer, sockfd);
            }
            // 处理信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) {
                bool flag = deal_signal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            // 处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN) {
                deal_read(sockfd);
            } else if (events[i].events & EPOLLOUT) {
                deal_write(sockfd);
            }
        }
        if (timeout) {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}