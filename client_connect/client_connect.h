//
// Created by shotacon on 23-3-15.
//

#ifndef YY_SERVER_CLIENT_CONNECT_H
#define YY_SERVER_CLIENT_CONNECT_H


#include "../config.h"

#include <netinet/in.h>
#include "sys/epoll.h"
#include "sys/socket.h"
#include "cstring"
#include "cerrno"
#include "unistd.h"

class Epoll_Net;

class Client_Connect{
public:
    Client_Connect(Epoll_Net* epoll_net, int client);

    ~Client_Connect();


public:
    int m_client_fd;
    //sockaddr_in m_client_address;
    Epoll_Net* m_epoll_net;

private:
    //接收发送缓冲层
    char m_read_buff[_READ_SIZE];
    int m_read_idx;
    char m_write_buff[_WRITE_SIZE];
    int m_write_idx;
};

#endif //YY_SERVER_CLIENT_CONNECT_H
