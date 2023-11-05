#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include "../lock/locker.h"
#include <queue>
using namespace std;

template <class T> class block_queue {
  public:
    block_queue(int max_size = 1000) {
        if (max_size <= 0) {
            throw std::exception();
        }

        m_max_size = max_size;
    }

    void clear() {
        m_mutex.lock();
        m_max_size = 0;
        while (!m_queue.empty()) {
            m_queue.pop();
        }
        m_mutex.unlock();
    }

    ~block_queue() {}
    // 判断队列是否满了
    bool full() {
        m_mutex.lock();
        if (m_queue.size() >= m_max_size) {

            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    // 判断队列是否为空
    bool empty() {
        m_mutex.lock();
        if (m_queue.empty()) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    // 返回队首元素
    bool front(T &value) {
        m_mutex.lock();
        if (m_queue.empty()) {
            m_mutex.unlock();
            return false;
        }
        value = m_queue.front();
        m_mutex.unlock();
        return true;
    }
    // 返回队尾元素
    bool back(T &value) {
        m_mutex.lock();
        if (m_queue.empty()) {
            m_mutex.unlock();
            return false;
        }
        value = m_queue.back();
        m_mutex.unlock();
        return true;
    }

    int size() {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_queue.size();

        m_mutex.unlock();
        return tmp;
    }

    int max_size() {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_max_size;

        m_mutex.unlock();
        return tmp;
    }
    // 往队列添加元素，需要将所有使用队列的线程先唤醒
    // 当有元素push进队列,相当于生产者生产了一个元素
    // 若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item) {

        m_mutex.lock();
        if (m_queue.size() >= m_max_size) {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }

        m_queue.push(item);
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    // pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item) {

        m_mutex.lock();
        while (m_queue.empty()) {

            if (!m_cond.wait(m_mutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }

        item = m_queue.front();
        m_queue.pop();
        m_mutex.unlock();
        return true;
    }

  private:
    locker m_mutex;
    cond m_cond;

    queue<T> m_queue;
    int m_max_size;
};

#endif