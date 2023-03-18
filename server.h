//
// Created by shotacon on 23-3-14.
//

#ifndef YY_SERVER_SERVER_H
#define YY_SERVER_SERVER_H

#include "thread_pool/thread_pool.h"
#include "client_connect/client_connect.h"
#include "sql/sql_connect_pool.h"


using namespace std;

class Server{
public:
    Server();

    void Init(int thread_max, int thread_min, int task_max, int sql_num, string m_user, string passwd, string datebase);

    bool Open_server();

private:



private:


    Sql_Connection_Pool* m_sql_pool;

    Client_Connect* m_clients;
};
#endif //YY_SERVER_SERVER_H
