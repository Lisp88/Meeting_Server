//
// Created by shotacon on 23-3-17.
//

#ifndef YY_SERVER_EPOLL_NET_H
#define YY_SERVER_EPOLL_NET_H

#include "../thread_pool/thread_pool.h"
#include "../config.h"
#include "../lock/lock_map.h"


#include "fcntl.h"
#include "stdexcept"
#include "map"
#include <netinet/in.h>
#include "sys/epoll.h"
#include "sys/socket.h"
#include "cstring"
#include "cerrno"
#include "unistd.h"

using namespace std;

class Epoll_Net;

class Data_Package{
public:
    Data_Package(Epoll_Net* epoll_net, int sock, char* buff, int len):
            m_epoll_net(epoll_net), m_sock(sock), m_buff(buff), m_len(len){}
    ~Data_Package(){
        if(m_buff) delete[] m_buff;
    }
    Epoll_Net* m_epoll_net;
    int m_sock;
    char* m_buff;
    int m_len;
};

class MyEvent{
public:
    MyEvent(Epoll_Net* epoll_net, int epoll_fd, int fd){
        m_fd = fd;
        m_epoll_fd = epoll_fd;
        m_epoll_net = epoll_net;
        state = false;
        m_event = 0;
    }

    void addEvent(int event){
        struct epoll_event ev = {0, {0}};
//        bzero(&ev, sizeof(ev));
        ev.data.fd = m_fd;
        ev.data.ptr = this;
        ev.events = m_event = event;
        int op;
        if(!state){
            op = EPOLL_CTL_ADD;
            state = true;
        }
        else
            op = EPOLL_CTL_MOD;

        int ret = epoll_ctl(m_epoll_fd, op, m_fd, &ev);

        if(ret == -1)
            printf("add epoll fd fail\n");
//        if(errno == ENOENT) printf("das");
    }

    void delEvent(){
        struct epoll_event ev;
        bzero(&ev, sizeof(ev));
        ev.data.ptr = this;
        state = false;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_fd, &ev);
    }

    Epoll_Net* m_epoll_net;
    int m_fd;
    int m_event;  //事件
    bool state; //是否被监听
    int m_epoll_fd;
};

class Epoll_Net{
public:
    Epoll_Net():m_epoll(0), m_listen_fd(0){}

    void Init(int thread_max, int thread_min, int task_max, void (*call_back)(int, char*, int));

    void Listen_events();

    void Loop_listen();

    //客户端读写处理(不是真正的处理流程，加入线程池)
    void Deal_read(MyEvent* ev);
    void Send_data(int fd, char* szbuf, int nlen);

private:
    bool Accept_client();

    //文件属性修改
    void Setnoblock(int fd);

    void Setnodelay(int fd);

    void Setreadbuff(int fd);

    void Setwritebuff(int fd);

    //线程池参数
    static void* Read_data(void*);

//    static void* Write_data(void*);

    static void* Package_deal(void*);

    //接收包之后解析协议的回调函数
    void (*m_call_back)(int, char*, int);

private:
    int m_epoll;

    int m_listen_fd;
    MyEvent* m_listen_event;

    struct epoll_event m_events[_MAX_LISTEN_EVENT];
    Lock_Map<int, MyEvent*> m_map_sock_to_event;

    Thread_Pool* m_thread_pool;
};



#endif //YY_SERVER_EPOLL_NET_H
