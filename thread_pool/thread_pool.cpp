#include "thread_pool.h"


Thread_Pool::Thread_Pool(int thread_max, int thread_min, int task_max)
:m_thread_max(thread_max), m_thread_min(thread_min), m_task_max(task_max){
    //参数判断
    if(thread_max <= 0 || thread_min > thread_max || task_max <=0){
        std::logic_error("thread_pool >> Thread Pool arg error");
    }

    //分配线程池空间
    tids = new pthread_t[thread_max];
    if(!tids) std::logic_error("thread_pool >> new tids error");

    m_thread_alive = 0;
    m_thread_busy = 0;
    m_pool_shutdown = false;
    m_thread_wait = 0;

    //消费线程池的线程创建
    int err;
    for (int i = 0; i < m_thread_min; i++) {
        if ((err = pthread_create(tids+i, nullptr, Custom, this)) > 0) {
           std::logic_error("thread_pool >> create custom thread error");
        }
        ++(m_thread_alive);
    }
    std::cout<<"custom thread create"<<std::endl;

    //管理者线程池创建
    if ((err = pthread_create(&manager_tid, nullptr, Manager, this)) > 0)
    {
        std::logic_error("thread_pool >> create manage thread error");
    }else
        std::cout<<"manager thread create"<<std::endl;
}


int Thread_Pool::Producer(void *(*p_fun)(void *), void * arg) {
    task_t t;
    t.task = p_fun;
    t.arg = arg;

    lock.lock();
    if(m_pool_shutdown)
    {
        lock.unlock();
        return -1;
    }

    //队列满了，挂起线程到条件变量上
    if(m_task_que.size() >= m_task_max){
        full.wait(lock.get());
    }

    m_task_que.push(t);

    empty.signal();
    lock.unlock();
    return 0;
}

void * Thread_Pool::Custom(void * arg)
{
    auto* pool = static_cast<Thread_Pool*>(arg);
    task_t task;
    while(!pool->m_pool_shutdown)
    {
        pool->lock.lock();

        //任务队列空，挂起在empty上等待唤醒
        while(pool->m_task_que.size() == 0)
        {
            pool->empty.wait(pool->lock.get());
        }

        //线程被管理线程唤醒，wait的数量就是回收空闲线程的数量
        if(pool->m_thread_wait > 0 && pool->m_thread_alive > pool->m_thread_min)
        {
            --(pool->m_thread_wait);
            --(pool->m_thread_alive);
            pool->lock.unlock();
            std::cout<<"缩减一个线程"<<std::endl;
            pthread_exit(nullptr);
        }

        task.task = pool->m_task_que.front().task;
        task.arg = pool->m_task_que.front().arg;

        pool->full.signal();
        ++(pool->m_thread_busy);
        pool->m_task_que.pop();
        pool->lock.unlock();

        //执行核心工作
        task.task(task.arg);

        pool->lock.lock();
        --(pool->m_thread_busy);
        pool->lock.unlock();
    }
    return 0;
}

void *Thread_Pool::Manager(void *arg)
{
    auto* pool = static_cast<Thread_Pool*>(arg);
    int alive;
    int cur;
    int busy;

    while(!pool->m_pool_shutdown )
    {
        pool->lock.lock();
        alive = pool->m_thread_alive;
        busy = pool->m_thread_busy;
        cur = pool->m_task_que.size();
        pool->lock.unlock();

        //扩容线程池，繁忙/存活超过百分之八十，一次添加一个
        if ((cur > alive - busy || (float) busy / alive * 100 >= (float) 80) && pool->m_thread_max >= alive+_ADD_THREAD_NUM) {
            int count = _ADD_THREAD_NUM;
            std::cout<<"满足扩容条件"<<std::endl;
            for (int i = 0; i < pool->m_thread_max && count > 0; i++) {
                //遍历，如果没有线程占用则进行扩容
                if (pool->tids[i] == 0 || !pool->If_alive(pool->tids[i])) {
                    pool->lock.lock();
                    pthread_create(&pool->tids[i], nullptr, Custom, (void *) pool);
                    std::cout<<"扩充一个线程"<<std::endl;
                    ++pool->m_thread_alive;
                    pool->lock.unlock();
                    count--;
                }
            }
        }

        //缩减，空闲线程数比繁忙的一半还多，一次回收一个
        if(busy *2 < alive - busy && alive > _DE_THREAD_NUM && alive >= pool->m_thread_min)
        {
            std::cout<<"满足缩减条件"<<std::endl;
            pool->lock.lock();
            pool->m_thread_wait = _DE_THREAD_NUM;
            pool->lock.unlock();

            for(int i = 0; i < _DE_THREAD_NUM; ++i)
                pool->empty.signal();
        }

        sleep(_MANAGE_LOOP_INTERVAL);
    }

    return 0;
}

Thread_Pool::~Thread_Pool() {

}

bool Thread_Pool::If_alive(pthread_t tid) {
    if(pthread_kill(tid, 0) == -1){
        if(errno == ESRCH) return false;
    }
    return true;
}


