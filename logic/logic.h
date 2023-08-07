//
// Created by shotacon on 23-3-18.
//

#ifndef YY_SERVER_LOGIC_H
#define YY_SERVER_LOGIC_H
#include "../server/server.h"
#include "../epoll_net/epoll_net.h"
#include "iostream"
using std::string;


class Logic {
public:
    Logic(Server* server):m_server(server) {
        m_server = server;
        m_sql_pool = server->m_sql_pool;
        m_tcp = server->m_epoll_net;
    }

    //设置协议映射
    void Set_Protocol();
    //发送数据
    inline void Send_data(int sock_fd, char* buff, int len){
        m_tcp->Send_data(sock_fd, buff, len);
    }
    void get_user_info(int id);

    //注册
    void RegisterRq(int clientfd, char*szbuf, int nlen);
    //登录
    void LoginRq(int clientfd, char*szbuf, int nlen);
    //创建房间
    void create_room(int clientfd, char*szbuf, int nlen);
    //加入房间
    void join_room(int clientfd, char*szbuf, int nlen);
    //更改信息
    void update_info(int clientfd, char*szbuf, int nlen);
    //接收退出包
    void quit_room(int clientfd, char*szbuf, int nlen);
    //接收音频
    void audio_frame(int clientfd, char*szbuf, int nlen);
    //接收视频
    void video_frame(int clientfd, char*szbuf, int nlen);
    //音频注册
    void audio_reg(int clientfd, char* szbuf, int nlen);
    //视频注册
    void video_reg(int clientfd, char* szbuf, int nlen);
    //用户离线
    void user_offline(int clientfd, char* szbuf, int nlen);
    //聊天请求
    void chat_resend(int clientfd, char* szbuf, int nlen);
    //上传文件头
    void file_upload(int clientfd, char* szbuf, int nlen);
    //文件块请求。写
    void file_content_rq(int clientfd, char* szbuf, int nlen);
    //下载文件头
    void file_download_rq(int clientfd, char* szbuf, int nlen);
    //文件块回复，读
    void file_content_rs(int clientfd, char* szbuf, int nlen);
private:
    Server* m_server;
    Sql_Connection_Pool* m_sql_pool;
    Epoll_Net* m_tcp;
    Lock_Map<int, UserInfo*> m_map_id_to_userinfo;
    Lock_Map<int, list<int>> m_map_room_to_userlist;
    Lock_Map<int, list<STRU_CHAT_RQ*>> m_map_room_chatinfo;
    Lock_Map<int64_t, FileInfo*> m_map_time_fileinfo;//key为用户id+时间戳，因为同一时间一个用户只能发一个文件
};


#endif //YY_SERVER_LOGIC_H
