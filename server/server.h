#ifndef YY_SERVER_SERVER_H
#define YY_SERVER_SERVER_H

#include "../epoll_net/epoll_net.h"
#include "../sql/sql_connect_pool.h"


#include "functional"


#define NetPackMap(a)  Server::Get_instance()->m_protocol_map_table[ a - DEF_PACK_BASE ]
class Logic;

typedef void (Logic::*P_FUN)(int, char*, int);

class Server{
public:
    static Server* Get_instance(){
        //局部变量初始化
        static Server server;
        return &server;
    }

    bool Open_server(int thread_max, int thread_min, int task_max, int sql_max);

    void Loop_events();

    //将包进行处理并按照协议分发工作
    static void Deal_data(int sock, char *buff, int len);

private:
    Server(){}
    ~Server(){}

    //设置协议映射
    void Set_protocol_map();
private:
    Epoll_Net* m_epoll_net;
    Sql_Connection_Pool* m_sql_pool;
    Logic* m_logic;

    friend class Logic;

    //协议表，存储函数地址
    P_FUN m_protocol_map_table[100];
};
#endif //YY_SERVER_SERVER_H