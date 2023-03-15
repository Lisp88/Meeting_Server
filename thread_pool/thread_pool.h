//
// Created by shotacon on 23-3-14.
//

#ifndef YY_SERVER_THREAD_POOL_H
#define YY_SERVER_THREAD_POOL_H
#include "../config.h"
#include "../lock/lock.h"

#include "iostream"
#include "csignal"
#include "queue"
#include "stdexcept"

typedef struct {
    void *(*task)(void *);

    void *arg;
} task_t;

class Thread_Pool {
public:
    Thread_Pool(int thread_max, int thread_min, int task_max);

    ~Thread_Pool();
    //pool_t *Pool_create(int, int, int);

    int Pool_destroy();

    int Producer(void *(*)(void *), void *);

    bool If_alive(pthread_t tid);

    static void *Custom(void *);

    static void *Manager(void *);

private:
    int m_thread_max;
    int m_thread_min;
    int m_thread_alive;//存在的线程数
    int m_thread_busy;//正在工作的线程数
    bool m_pool_shutdown;
    int m_thread_wait;
    int m_task_max;

    cond full;
    cond empty;
    locker lock;
    std::queue<task_t> m_task_que;
    pthread_t *tids;
    pthread_t manager_tid;
};

#endif //YY_SERVER_THREAD_POOL_H
