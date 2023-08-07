//
// Created by shotacon on 23-7-30.
//
#include "server/server.h"

int main() {
    setbuf(stdout, 0);

    Server* server = Server::Get_instance();

    server->Open_server(20, 10, 10000, 10);

    server->Loop_events();

    return 0;
}