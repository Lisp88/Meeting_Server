//
// Created by shotacon on 23-3-17.
//

#ifndef YY_SERVER_EPOLL_NET_H
#define YY_SERVER_EPOLL_NET_H

#include "../thread_pool/thread_pool.h"
#include "../client_connect/client_connect.h"
#include "../config.h"
#include "../lock/lock_map.h"

#include "fcntl.h"
#include "stdexcept"
#include "map"

using namespace std;


class Epoll_Net{
public:
    void Init(int thread_max, int thread_min, int task_max);

    void Listen_events();

    void Loop_listen();
private:

    //监听树的节点控制
    void Add_fd(int fd, bool ET, bool one_shot);

    void Del_fd(int fd);

    bool Accept_client(int client_fd);

    //文件属性修改
    void Setnoblock(int fd);

    void Setnodelay(int fd);

    void Setreadbuff(int fd);

    void Setwritebuff(int fd);

    //客户端读写处理
    void Deal_read(Client_Connect* client_info);

    void Deal_write(Client_Connect* client_info);

    static void* Read_data(void*);

    static void* Write_data(void*);

    static void* Package_deal(void*);

    //接收包之后解析协议的回调函数
    void (*call_back)(int, char*, int);

private:
    int m_epoll;
    int m_listen_fd;
    epoll_event m_events[_MAX_LISTEN_EVENT];

    map<int, Client_Connect* > m_socket_to_client_conn;

    Thread_Pool* m_thread_pool;
};

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

#endif //YY_SERVER_EPOLL_NET_H
