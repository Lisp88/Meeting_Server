//
// Created by shotacon on 23-3-15.
//

#ifndef YY_SERVER_CONFIG_H
#define YY_SERVER_CONFIG_H

//服务器
#define _PORT 6666

//数据库
#define _DB_IP "localhost"

#define _DB_NAME "yy"

#define _DB_PASSWD "88888888"

#define _DB_USER "root"


//线程池
#define _MANAGE_LOOP_INTERVAL 5

#define _ADD_THREAD_NUM 3

#define _DE_THREAD_NUM 3

//epoll
#define _LT         0

#define _READ_SIZE 2048     //缓冲层大小

#define _WRITE_SIZE 2048

#define _READ_BUFF_SIZE 256*1024    //内核缓冲区大小

#define _WRITE_BUFF_SIZE 128*10242

#define _MAX_LISTEN_EVENT 10000

#endif //YY_SERVER_CONFIG_H

//logic protocol

