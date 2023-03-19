//
// Created by shotacon on 23-3-18.
//

#ifndef YY_SERVER_LOGIC_H
#define YY_SERVER_LOGIC_H
#include "../server.h"


class Logic {
public:
    Logic(Server* server):m_server(server) {

    }

    //设置协议映射
    void Set_Protocol();
private:
    Server* m_server;
};


#endif //YY_SERVER_LOGIC_H
