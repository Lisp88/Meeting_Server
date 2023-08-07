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
    bzero(&address, sizeof(sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(_PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    //设置重复绑定
    int flag = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));

    if(bind(m_listen_fd, (sockaddr *)&address, sizeof(sockaddr_in))){
        close(m_listen_fd);
        throw logic_error("server >> bind fail");
    }

    if(listen(m_listen_fd, 128) == -1){
        close(m_listen_fd);
        throw logic_error("server >> listen fail");
    }
    //epoll
    m_epoll = epoll_create(_MAX_LISTEN_EVENT);
    m_listen_event = new MyEvent(this, m_epoll, m_listen_fd);
    m_listen_event->addEvent(EPOLLIN);
}

void Epoll_Net::Loop_listen() {
    while(1){
        int number = epoll_wait(m_epoll, m_events, _MAX_LISTEN_EVENT, 1000);
        if(number <= 0) continue;
        for(int i = 0; i < number; ++i){
            MyEvent *ev = (MyEvent*)m_events[i].data.ptr;
            int fd = ev->m_fd;
            //客户端读操作
            if(m_events[i].events & EPOLLIN) {
                //客户端连接
                if (fd == m_listen_fd)
                    Accept_client();
                else {
                    Deal_read(ev);
                }
            }
            if(m_events[i].events & EPOLLOUT){//客户端写操作
                //printf("server send to client %d data, write data", sock_fd);
            }
        }
    }
}


bool Epoll_Net::Accept_client() {
    struct sockaddr_in client_address;
    socklen_t sock_len = sizeof(client_address);

    int con_fd = accept(m_listen_fd, (struct sockaddr*)&client_address, &sock_len);

    if(con_fd < 0) {
        cout << "server >> accept return con_fd < 0" << endl;
        return false;
    }

    //设置内核读写缓冲区
    Setreadbuff(con_fd);
    Setwritebuff(con_fd);
    //禁用nagle算法
    Setnodelay(con_fd);

#ifdef _LT
    MyEvent* ev = new MyEvent(this, m_epoll, con_fd);
    ev->addEvent(EPOLLIN|EPOLLONESHOT);
    m_map_sock_to_event.Insert(con_fd, ev);

    printf("server accept client :%d connection\n", con_fd);
#else

#endif

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



void Epoll_Net::Deal_read(MyEvent* ev) {
    m_thread_pool->Producer(Read_data, (void*)ev);
}

    //在ＬＴ阻塞模式下不注册写事件的原因．
    //1. 消耗大量ＩＯ，如果写缓冲区还可以写，则一直会触发事件
    //2. 一般只有在缓冲区满后，才注册ｏｕｔ事件，等待发送．阻塞ＬＴ模式下若包过大，则会阻塞并且一直接收到触发事件．而非阻塞ＥＴ下可以正常使用．


void *Epoll_Net::Read_data(void * arg) {
    MyEvent* ev = (MyEvent*)arg;
    Epoll_Net* pthis = ev->m_epoll_net;

#ifdef _LT
    int nRelReadNum = 0;
    int nPackSize = 0;
    char *pSzBuf = nullptr;
    do
    {
        nRelReadNum = read(ev->m_fd,&nPackSize,sizeof(nPackSize) );
        if(nRelReadNum <= 0)
            break;

        pSzBuf = new char[nPackSize];
        int nOffSet = 0;
        nRelReadNum = 0;
        //接收包的数据
        while(nPackSize)
        {
            nRelReadNum = recv(ev->m_fd,pSzBuf+nOffSet,nPackSize,0);
            if(nRelReadNum <= 0)
                break;

            nOffSet += nRelReadNum;
            nPackSize -= nRelReadNum;
        }
//        printf("epoll_net >> read_data >> data : %s\n", buff);
        Data_Package* data_package = new Data_Package(ev->m_epoll_net, ev->m_fd, pSzBuf, nOffSet);
        pthis->m_thread_pool->Producer(Package_deal, data_package);

        //继续监听读事件
        ev->addEvent(ev->m_event);

        return nullptr;
    }while(0);
#elif

#endif
    //进行错误处理，删除内存，下监听树
    ev->delEvent();
    close(ev->m_fd);
    pthis->m_map_sock_to_event.Del(ev->m_fd);
    delete ev;

    return nullptr;
}

void *Epoll_Net::Package_deal(void * arg) {

//    printf("package deal \n");
    if(!arg) return nullptr;
    Data_Package* package = static_cast<Data_Package*>(arg);
    package->m_epoll_net->m_call_back (package->m_sock, package->m_buff, package->m_len);

    if(package)
        delete package;
    return nullptr;
}

void Epoll_Net::Send_data(int fd, char *szbuf, int nlen) {
    int nPackSize = nlen + 4;
    vector<char> vecbuf( nPackSize , 0);

    char* buf = &* vecbuf.begin();
    char* tmp = buf;
    *(int*)tmp = nlen;//按四个字节int写入
    tmp += sizeof(int );
    memcpy( tmp , szbuf , nlen );

    int res = send( fd,(const char *)buf, nPackSize ,0);
}

