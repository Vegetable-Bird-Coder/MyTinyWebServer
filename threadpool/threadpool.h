#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <list>
#include <pthread.h>

#include "../lock/locker.h"
#include "../mysql/sql_conn_pool.h"

template <typename T> class ThreadPool {
  public:
    ThreadPool(int actor_model, SqlPool *sql_pool, int thread_num = 8,
               int max_request_num = 10000);
    ~ThreadPool();
    bool append(T *request, int state);
    bool append(T *request);

  private:
    static void *worker(void *arg);
    void run();

  private:
    int m_thread_num;
    int m_max_request_num;
    pthread_t *m_threads;
    std::list<T *> m_workqueue;
    locker m_queue_locker;
    sem m_queue_stat;
    SqlPool *m_sql_pool;
    int m_actor_model;
};

template <typename T>
ThreadPool<T>::ThreadPool(int actor_model, SqlPool *sql_pool, int thread_num,
                          int max_request_num)
    : m_actor_model(actor_model), m_sql_pool(sql_pool),
      m_thread_num(thread_num), m_max_request_num(max_request_num) {
    if (thread_num <= 0 || max_request_num <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_num];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_num; i++) {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception;
        }
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception;
        }
    }
}

template <typename T> ThreadPool<T>::~ThreadPool() { delete[] m_threads; }

template <typename T> bool ThreadPool<T>::append(T *request) {
    m_queue_locker.lock();
    if (m_workqueue.size() >= m_max_request_num) {
        m_queue_locker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queue_locker.unlock();
    m_queuestat.post();
    return true;
}

template <typename T> bool ThreadPool<T>::append(T *request, int state) {
    request->m_state = state;
    return append(request);
}

template <typename T> void *ThreadPool<T>::worker(void *arg) {
    ThreadPool *thread_pool = (ThreadPool *)arg;
    thread_pool->run();
    return thread_pool;
}

template <typename T> void ThreadPool<T>::run() {
    while (true) {
        m_queuestat.wait();
        m_queue_locker.lock();
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queue_locker.unlock();
        if (!request)
            continue;
        if (m_actor_model == 1) { // reactor
            if (request->m_state == 0) {
                if (request->read()) {
                    request->io_finish = true;
                    SqlConnRAII sql_conn_raii(&request->sql_conn, m_sql_pool);
                    request->process();
                } else {
                    request->timer_flag = true; // 告诉主线程需要启用计时器回调函数
                    request->io_finish = true; // 告诉主线程IO完成，因为reactor模式下，主线程不知道异步的IO是否成功
                }
            } else {
                if (request->write()) {
                    request->io_finish = true;
                } else {
                    request->timer_flag = true;
                    request->io_finish = true;
                }
            }
        } else { // proactor
            SqlConnRAII sql_conn_raii(&request->sql_conn, m_sql_pool);
            request->process();
        }
    }
}

#endif