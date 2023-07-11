//
// Created by shotacon on 23-3-14.
//

#include "server.h"
#include "logic/logic.h"


void Server::Deal_data(int sock, char *buff, int len) {
    Data_Package* package = static_cast<Data_Package*>((void*)buff);
    int type = *(int*)package;
    cout<<"protocol number : "<<type<<endl;
    P_FUN pf = NetPackMap(type);
    if(pf)
        (Server::Get_instance()->m_logic->*pf)(sock, buff, len);
}

bool Server::Open_server(int thread_max, int thread_min, int task_max, int sql_max) {
    //数据库连接池初始化，并验证链接
    m_sql_pool = Sql_Connection_Pool::Get_instance();
    m_sql_pool->init(_DB_IP, 3306, _DB_USER, _DB_PASSWD, _DB_NAME, sql_max);

    //epoll网络模型初始化，包含线程池的初始化
    m_epoll_net = new Epoll_Net;
    m_epoll_net->Init(thread_max, thread_min, task_max, Get_instance()->Deal_data);
    m_epoll_net->Listen_events();
    //逻辑类创建
    m_logic = new Logic(this);
    //设置协议映射
    Set_protocol_map();
    return false;
}

void Server::Set_protocol_map() {
    m_logic->Set_Protocol();
}

void Server::Loop_events() {
    m_epoll_net->Loop_listen();
}
