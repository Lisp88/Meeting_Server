//
// Created by shotacon on 23-3-15.
//


#include "client_connect.h"


Client_Connect::Client_Connect(Epoll_Net* epoll_net, int client) :m_read_idx(0), m_write_idx(0), m_epoll_net(epoll_net), m_client_fd(client) {
    memset(m_read_buff, 0, sizeof (m_read_buff));
    memset(m_write_buff, 0, sizeof (m_write_buff));
}


