//
// Created by shotacon on 23-3-17.
//

#include "epoll_net.h"

void Epoll_Net::Init(int thread_max, int thread_min, int task_max, void (*call_back)(int, char*, int)) {
    m_call_back = call_back;
    //创建线程池
    m_thread_pool = new Thread_Pool(thread_max, thread_min, task_max);
}

void Epoll_Net::Listen_events() {
    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    //设置重复绑定
    int flag = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));

    if(bind(m_listen_fd, (sockaddr *)&address, sizeof(address)))
        throw logic_error("server >> bind fail");
    if(listen(m_listen_fd, 128))
        throw logic_error("server >> listen fail");

    //epoll
    m_epoll = epoll_create(5);
    Add_fd(m_listen_fd, false, false);
}

void Epoll_Net::Loop_listen() {
    bool server_stop = false;

    while(!server_stop){
        int number = epoll_wait(m_epoll, m_events, _MAX_LISTEN_EVENT, 1000);

        for(int i = 0; i < number; ++i){
            int sock_fd = m_events[i].data.fd;
            //客户端连接
            if(sock_fd == m_listen_fd){
                Accept_client();
            }
                //客户端读操作
            else if(m_events[i].events & EPOLLIN){
                printf("server recv client %d data, read data\n", sock_fd);
                Deal_read(m_socket_to_client_conn[sock_fd]);
            }
                //客户端写操作
            else if(m_events[i].events & EPOLLOUT){
                printf("server send to client %d data, write data", sock_fd);
                Deal_write(m_socket_to_client_conn[sock_fd]);
            }
        }
    }
}


bool Epoll_Net::Accept_client() {
    struct sockaddr_in client_address;
    socklen_t sock_len = sizeof(client_address);

    int con_fd = accept(m_listen_fd, (sockaddr*)&client_address, &sock_len);

    if(con_fd < 0) {
        cout << "server >> accept return con_fd < 0" << endl;
        return false;
    }
    printf("server accept client :%d connection\n", con_fd);
    //创建客户端信息，并与套接字进行映射
    Client_Connect* p_client_info = new Client_Connect(this, con_fd);
    m_socket_to_client_conn[con_fd] = p_client_info;

    //禁用nagle算法
    Setnodelay(con_fd);

    //设置阻塞
#ifdef _LT
    //挂载客户端节点到监听树上
    Add_fd(con_fd, false, true);

#else
    Setnoblock(con_fd);
    //挂载客户端节点到监听树上
    Add_fd(con_fd, true, true);
#endif

    //设置内核读写缓冲区
    Setreadbuff(con_fd);
    Setwritebuff(con_fd);

    return true;
}

void Epoll_Net::Setnoblock(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;

    fcntl(fd, F_SETFL, new_opt);
}

#include <netinet/tcp.h>
void Epoll_Net::Setnodelay(int fd) {
    int value = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(int));
}

void Epoll_Net::Setreadbuff(int fd) {
//接收缓冲区
    int nRecvBuf = _READ_BUFF_SIZE;//设置为 256 K
    setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
}

void Epoll_Net::Setwritebuff(int fd) {
//发送缓冲区
    int nSendBuf = _WRITE_BUFF_SIZE;//设置为 128 K
    setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
}

void Epoll_Net::Add_fd(int fd, bool ET, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;

    if(one_shot) event.events |= EPOLLONESHOT;

    if(ET) event.events |= EPOLLET;

    epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &event);
}

void Epoll_Net::Del_fd(int fd) {
    epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, nullptr);
}

void Epoll_Net::Deal_read(Client_Connect *client_info) {
    m_thread_pool->Producer(Read_data, client_info);
}

void Epoll_Net::Deal_write(Client_Connect *client_info) {
    //m_thread_pool->Producer(Write_data, client_info);
}

void *Epoll_Net::Read_data(void * arg) {
    Client_Connect* client_info = static_cast<Client_Connect*>(arg);
    int client_fd = client_info->m_client_fd;
#ifdef _LT
    int pack_size = 0;
    int read_res = 0;
    char *buff;
    do{
        read_res = read(client_fd, &pack_size, sizeof(int));
        if(read_res <= 0){
            cout<<"epoll_net >> read package size is 0"<<endl;
            break;
        }

        buff = new char[pack_size];
        int read_bytes = 0;
        read_res = 0;
        while(read_bytes < pack_size){
            read_res = read(client_fd, buff, pack_size-read_bytes);
            if(read_res <= 0) break;
            read_bytes += read_res;
        }
        printf("epoll_net >> read_data >> data : %s\n", buff);
        Data_Package* data_package = new Data_Package(client_info->m_epoll_net, client_fd, buff, pack_size);
        client_info->m_epoll_net->m_thread_pool->Producer(Package_deal, data_package);

        //继续监听读事件
        client_info->m_epoll_net->Add_fd(client_fd, false, true);

        return nullptr;
    }while(0);
#elif

#endif
    //进行错误处理，删除内存，下监听树
    client_info->m_epoll_net->Del_fd(client_fd);
    close(client_fd);
    client_info->m_epoll_net->m_socket_to_client_conn.erase(client_fd);
    delete buff;
    return nullptr;
}

void *Epoll_Net::Write_data(void * arg) {



    return nullptr;
}

void *Epoll_Net::Package_deal(void * arg) {

    printf("package deal \n");
    if(!arg) return nullptr;
    Data_Package* package = static_cast<Data_Package*>(arg);
    package->m_epoll_net->m_call_back (package->m_sock, package->m_buff, package->m_len);

    if(package)
        delete package;
    return nullptr;
}

