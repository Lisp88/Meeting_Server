////
//// Created by shotacon on 23-3-18.
////
//
#include "logic.h"
#include "errno.h"

void Logic::Set_Protocol() {
    NetPackMap(DEF_PACK_REGISTER_RQ) = &Logic::RegisterRq;
    NetPackMap(DEF_PACK_LOGIN_RQ) = &Logic::LoginRq;
    NetPackMap(DEF_PACK_CREATEROOM_RQ) = &Logic::create_room;
    NetPackMap(DEF_PACK_JOINROOM_RQ) = &Logic::join_room;
    NetPackMap(DEF_PACK_USER_INFO) = &Logic::update_info;
    NetPackMap(DEF_PACK_LEAVEROOM_RQ) = &Logic::quit_room;
    NetPackMap(DEF_PACK_AUDIO_FRAME) = &Logic::audio_frame;
    NetPackMap(DEF_PACK_VIDEO_FRAME) = &Logic::video_frame;
    NetPackMap(DEF_PACK_AUDIO_REGISTER) = &Logic::audio_reg;
    NetPackMap(DEF_PACK_VIDEO_REGISTER) = &Logic::video_reg;
    NetPackMap(DEF_PACK_USER_OFFLINE) = &Logic::user_offline;
    NetPackMap(DEF_PACK_CHAT_RQ) = &Logic::chat_resend;
    NetPackMap(DEF_PACK_UPLOAD_FILE_RQ) = &Logic::file_upload;
    NetPackMap(DEF_PACK_FILE_CONTENT_RQ) = &Logic::file_content_rq;
    NetPackMap(DEF_PACK_FILE_CONTENT_RS) = &Logic::file_content_rs;
    NetPackMap(DEF_PACK_FILE_DOWNLOAD_RQ) = &Logic::file_download_rq;
    NetPackMap(DEF_PACK_OFFLINE) = &Logic::user_offline;
}

void Logic::audio_reg(int clientfd, char *szbuf, int nlen) {
    printf("clientfd %d audio register\n", clientfd);
    STRU_AUDIO_REGISTER* rq = (STRU_AUDIO_REGISTER*)szbuf;
    UserInfo * user = nullptr;
//    if(!m_map_id_to_userinfo.find(rq->m_userid, user)) return;
    if(m_map_id_to_userinfo.Is_exist(rq->m_userid))
        m_map_id_to_userinfo.Find(rq->m_userid, user);
    user->m_audiofd = clientfd;
}

void Logic::video_reg(int clientfd, char *szbuf, int nlen) {
    printf("clientfd %d video register\n", clientfd);
    STRU_VIDEO_REGISTER* rq = (STRU_VIDEO_REGISTER*)szbuf;
    UserInfo * user = nullptr;
//    if(!m_map_id_to_userinfo.find(rq->m_userid, user)) return;
    if(m_map_id_to_userinfo.Is_exist(rq->m_userid))
        m_map_id_to_userinfo.Find(rq->m_userid, user);
    user->m_videofd = clientfd;
}

//注册
void Logic::RegisterRq(int clientfd, char *szbuf, int nlen) {
    printf("clientfd:%d RegisterRq\n", clientfd);
    STRU_REGISTER_RS register_rs;
    STRU_REGISTER_RQ *register_rq = (STRU_REGISTER_RQ *) szbuf;
    //查询
    char sql_szbuf[1024]{};
    list<string> query_result;
    sprintf(sql_szbuf, "select telephone "
                       "from user "
                       "where telephone = '%s';", register_rq->m_tel);
    Connection_RAII sql(m_sql_pool);
    if (!sql.Select_mysql(sql_szbuf, 1, query_result)) {
        std::cout << "select tel fail" << std::endl;
    }
    if (query_result.size()) {
        register_rs.m_lResult = tel_is_exist;
    } else {
        query_result.clear();
        sprintf(sql_szbuf, "select name from user where name = '%s';", register_rq->m_name);
        if (!sql.Select_mysql(sql_szbuf, 1, query_result)) {
            std::cout << "select name fail" << std::endl;
        }
        if (query_result.size()) {
            register_rs.m_lResult = name_is_exist;
        } else {
            register_rs.m_lResult = register_success;
//            std::uniform_int_distribution<int> u(0, 35);
            srand((int) time(0));
            sprintf(sql_szbuf, "insert into user(telephone, password, name, icon, feeling) "
                               "values('%s', '%s', '%s', %d, '普通用户');",
                    register_rq->m_tel, register_rq->m_password, register_rq->m_name, rand() % 35);
            if (!sql.Update_mysql(sql_szbuf)) {
                std::cout << "update fail : " << sql_szbuf << std::endl;
            }
        }
    }
    Send_data(clientfd, (char *) &register_rs, sizeof(register_rs));
}

//登录
void Logic::LoginRq(int clientfd, char *szbuf, int nlen) {
    printf("clientfd:%d LoginRq\n", clientfd);
    //拆包
    STRU_LOGIN_RQ *login_rq = (STRU_LOGIN_RQ *) szbuf;
    std::cout << login_rq->m_tel << ": login_rq" << std::endl;
    STRU_LOGIN_RS login_rs;
    list<string> do_result;
    char sql_buff[1024] = "";
    Connection_RAII sql(m_sql_pool);
    sprintf(sql_buff, "select id, password from user where telephone = '%s';", login_rq->m_tel);
    if (!sql.Select_mysql(sql_buff, 2, do_result)) {
        cout << "deal_login_data: >> query tel and password fail" << endl;
    }
    if (do_result.size()) {
        int id = atoi(do_result.front().c_str());
        do_result.pop_front();
        string password = do_result.front();
        do_result.pop_front();
        if (strcmp(password.c_str(), login_rq->m_password) == 0) {
            login_rs.m_lResult = login_success;
            login_rs.m_userid = id;

            UserInfo *user = nullptr;
            if(m_map_id_to_userinfo.Is_exist(id)){
                //查到
                goto label;
                //强制让人下线
                //delete user;
            }
            //没查到
            m_map_id_to_userinfo.Find(id, user);
            user = new UserInfo;

            user->m_id = id;
            user->m_sockfd = clientfd;

            m_map_id_to_userinfo.Insert(id, user);
            Send_data(clientfd, (char *) &login_rs, sizeof(login_rs));
            //发送个人信息 更新状态
            get_user_info(id);
            return;
        } else {
            login_rs.m_lResult = password_error;
        }
    } else {
        //查无此人
        label : login_rs.m_lResult = user_not_exist;
    }
    //返回包信息
    Send_data(clientfd, (char *) &login_rs, sizeof(login_rs));

}

#include "random"
//创建房间
void Logic::create_room(int clientfd, char *szbuf, int nlen) {
    printf("clientfd:%d create_room\n", clientfd);
    STRU_JOINROOM_RQ *rq = (STRU_JOINROOM_RQ *) szbuf;
    default_random_engine e;
    uniform_int_distribution<unsigned> u(100000, 999999);
    int room_id = 0;
    do {
        room_id = u(e);
        printf("room id %d\n", room_id);

    }while (m_map_room_to_userlist.Is_exist(room_id));
    list<int> lst;
    lst.push_back(rq->m_UserID);

    list<STRU_CHAT_RQ*> chats;
    m_map_room_to_userlist.Insert(room_id, lst);
    if(!m_map_room_chatinfo.Is_exist(room_id)){
        m_map_room_chatinfo.Insert(room_id, chats);
    }

    //回复
    STRU_CREATEROOM_RS rs;
    rs.m_RoomId = room_id;
    rs.m_lResult = create_success;
    Send_data(clientfd, (char *) &rs, sizeof(rs));
}

//加入房间
void Logic::join_room(int clientfd, char *szbuf, int nlen) {
    printf("client : %d Join Room rq\n", clientfd);
    //拆包
    STRU_JOINROOM_RQ *rq = (STRU_JOINROOM_RQ *) szbuf;
    STRU_JOINROOM_RS rs;
    //查看房间是否存在
    list<int> lst;
    if(!m_map_room_to_userlist.Is_exist(rq->m_RoomID)){
        rs.m_lResult = room_no_exist;
    } else {
        rs.m_lResult = join_success;
        rs.m_RoomID = rq->m_RoomID;
    }
    m_map_room_to_userlist.Find(rq->m_RoomID, lst);
    Send_data(clientfd, (char *) &rs, sizeof(rs));
    //查看自身信息
    UserInfo *p_joinuser = nullptr;
    if(!m_map_id_to_userinfo.Is_exist(rq->m_UserID))
        return;
    else
        m_map_id_to_userinfo.Find(rq->m_UserID, p_joinuser);
    //构建房间请求
    STRU_ROOM_MEMBER_RQ room_joiner_rq;
    room_joiner_rq.m_UserID = rq->m_UserID;
    strcpy(room_joiner_rq.m_szUser, p_joinuser->m_userName);
    //获取房间成员列表
    for (auto ite = lst.begin(); ite != lst.end(); ++ite) {
        int member_id = *ite;
        UserInfo *p_user = nullptr;
        if(!m_map_id_to_userinfo.Is_exist(member_id))
            return;
        else
            m_map_id_to_userinfo.Find(member_id, p_user);
        STRU_ROOM_MEMBER_RQ room_member_rq;
        room_member_rq.m_UserID = member_id;
        strcpy(room_member_rq.m_szUser, p_user->m_userName);
        //发送加入者信息给房间成员
        Send_data(p_user->m_sockfd, (char *) &room_joiner_rq, sizeof(room_joiner_rq));
        //发送房间成员信息给加入者
        Send_data(p_joinuser->m_sockfd, (char *) &room_member_rq, sizeof(room_member_rq));
    }
    //将加入者添加到房间列表中
    lst.push_back(p_joinuser->m_id);
    m_map_room_to_userlist.Insert(rq->m_RoomID, lst);

    list<STRU_CHAT_RQ*> chats;
    if(m_map_room_chatinfo.Is_exist(rq->m_RoomID)){
        m_map_room_chatinfo.Find(rq->m_RoomID, chats);
    }

    //发送当前房间的聊天信息
    for(auto ite = chats.begin(); ite != chats.end(); ++ite){
        STRU_CHAT_RQ rq = **ite;
        int len = sizeof(rq);
        Send_data(p_joinuser->m_sockfd, (char*)&rq, sizeof(rq));
    }
    FileInfo *info = nullptr;
    //发送该用户下载的文件信息
    Connection_RAII sql(m_sql_pool);
    char sql_buf[1024]{0};
    sprintf(sql_buf, "select time_key from download_files where is_ok = false and use_id = %d", rq->m_UserID);
    list<string> lst_key;
    if(!sql.Select_mysql(sql_buf, 1, lst_key))
        printf("select mysql sql_buf is fail");
    while(!lst_key.empty()){
        int64_t time_key = stoll(lst_key.front());
        m_map_time_fileinfo.Find(time_key, info);
        STRU_UPLOAD_FILE_RQ file_rq;
        file_rq.userid = info->user_id;
        file_rq.timestamp = info->timestamp;
        file_rq.room_id = info->room_id;
        file_rq.size = info->size;
        strcpy(file_rq.md5, info->md5.c_str());
        strcpy(file_rq.time, info->time.c_str());
        strcpy(file_rq.fileName, info->name.c_str());
        strcpy(file_rq.dir, info->dir.c_str());
        Send_data(clientfd, (char*)&file_rq, sizeof(file_rq));
    }
    for(auto ite = m_map_time_fileinfo.m.begin(); ite != m_map_time_fileinfo.m.end(); ++ite){
        info = ite->second;
        if(info->room_id == rq->m_RoomID){

        }
    }
}

//修改信息
void Logic::update_info(int clientfd, char *szbuf, int nlen) {
    printf("client : %d update info rq\n", clientfd);

    //拆包
    STRU_USER_INFO_RQ *rq = (STRU_USER_INFO_RQ *) szbuf;
    printf("新名字: %s\n", rq->name);
    printf("新签名: %s\n", rq->feeling);
    Connection_RAII sql(m_sql_pool);
    //mysql 修改信息
    char sql_buff[1024] = "";
    sprintf(sql_buff, "update user set icon = %d, name = '%s', feeling = '%s' where id = %d;",
            rq->iconid, rq->name, rq->feeling, rq->userid);
    if (!sql.Update_mysql(sql_buff)) {
        printf("update info >> update db fail\n");
        return;
    }
    //发送信息给客户端
    get_user_info(rq->userid);
}

void Logic::get_user_info(int id) {
    //从数据库获取信息
    list<string> lst;
    char sql_buff[1024]{""};
    sprintf(sql_buff, "select name, icon, feeling from user where id = %d;", id);
    Connection_RAII sql(m_sql_pool);
    if (!sql.Select_mysql(sql_buff, 3, lst)) {
        printf("get user info >> select db fail\n");
        return;
    }
    if (lst.size() != 3) return;
    string name = lst.front();
    lst.pop_front();
    int icon = stoi(lst.front());
    lst.pop_front();
    string feeling = lst.front();
    lst.pop_front();
    //设置缓存信息
    UserInfo *user;
    if(m_map_id_to_userinfo.Is_exist(id)){
        m_map_id_to_userinfo.Find(id, user);
        strcpy(user->m_userName, name.c_str());
    }else return;
    //发送客户端
    STRU_USER_INFO_RQ rq;
    rq.userid = id;
    strcpy(rq.feeling, feeling.c_str());
    strcpy(rq.name, name.c_str());
    rq.iconid = icon;

    Send_data(user->m_sockfd, (char *) &rq, sizeof(rq));
}

//处理退出房间
void Logic::quit_room(int clientfd, char *szbuf, int nlen) {
    printf("client : %d quit room rq\n", clientfd);
    STRU_LEAVEROOM_RQ *rq = (STRU_LEAVEROOM_RQ *) szbuf;
    list<int> lst;
    if(m_map_room_to_userlist.Is_exist(rq->m_RoomId)) {
        m_map_room_to_userlist.Find(rq->m_RoomId, lst);
    }else return;
    //根据房间id 转发列表
    for (auto ite = lst.begin(); ite != lst.end();) {
        int id = *ite;
        if (id == rq->m_nUserId) {
            ite = lst.erase(ite);
        } else {
            //转发
            UserInfo *user = nullptr;
            if(m_map_id_to_userinfo.Is_exist(id)) {
                m_map_id_to_userinfo.Find(id, user);
            }else continue;
            Send_data(user->m_sockfd, (char *) rq, nlen);
            ++ite;
        }
    }
    list<STRU_CHAT_RQ*> chats;
    //更新缓存map
    if (lst.empty()) {
        m_map_room_to_userlist.Del(rq->m_RoomId);
        //回收聊天信息
        if(m_map_room_chatinfo.Is_exist(rq->m_RoomId)) {
            m_map_room_chatinfo.Find(rq->m_RoomId, chats);
            for(auto ite = chats.begin(); ite != chats.end(); ++ite){
                STRU_CHAT_RQ* rq =  *ite;
                delete rq;
            }
            m_map_room_chatinfo.Del(rq->m_RoomId);
            printf("clear %d room chats info\n", rq->m_RoomId);
        }
        //回收文件信息
        for(auto ite = m_map_time_fileinfo.m.begin(); ite != m_map_time_fileinfo.m.end(); ++ite){
            FileInfo *info = ite->second;
            if(info->room_id == rq->m_RoomId){
                m_map_time_fileinfo.Del(ite->first);
                Connection_RAII sql(m_sql_pool);
                char sql_buf[1024]{0};
                sprintf(sql_buf, "delete from ");
            }
        }
    } else {
        m_map_room_to_userlist.Insert(rq->m_RoomId, lst);
    }
}

//音频数据转发
void Logic::audio_frame(int clientfd, char *szbuf, int nlen) {
    printf("client : %d audio frame \n", clientfd);
    //拆包，反序列化
    char *temp = szbuf;
    temp += sizeof(int);
    int user_id = *(int *) temp;
    temp += sizeof(int);
    int room_id = *(int *) temp;
    //temp += sizeof(int);
    list<int> lst;
//    if (!m_map_room_to_userlist.find(room_id, lst)) return;
    if(m_map_room_to_userlist.Is_exist(room_id))
        m_map_room_to_userlist.Find(room_id, lst);
    else return;
    for (auto ite = lst.begin(); ite != lst.end(); ++ite) {
        int id = *ite;
        if (id != user_id) {
            //转发
            UserInfo *user = nullptr;
            if(m_map_id_to_userinfo.Is_exist(id))
                m_map_id_to_userinfo.Find(id, user);
            else return;
            Send_data(user->m_sockfd, szbuf, nlen);
        }
    }
}

//视频数据转发
void Logic::video_frame(int clientfd, char *szbuf, int nlen) {
    printf("client : %d video frame \n", clientfd);
    //拆包，反序列化
    char *temp = szbuf;
    temp += sizeof(int);
    int user_id = *(int *) temp;
    temp += sizeof(int);
    int room_id = *(int *) temp;
    //temp += sizeof(int);
    list<int> lst;
    if(m_map_room_to_userlist.Is_exist(room_id))
        m_map_room_to_userlist.Find(room_id, lst);
    else return;
    for (auto ite = lst.begin(); ite != lst.end(); ++ite) {
        int id = *ite;
        if (id != user_id) {
            //转发
            UserInfo *user = nullptr;
            if(m_map_id_to_userinfo.Is_exist(id))
                m_map_id_to_userinfo.Find(id, user);
            else return;
            Send_data(user->m_sockfd, szbuf, nlen);
        }
    }
}

void Logic::user_offline(int clientfd, char *szbuf, int nlen) {
    printf("client : %d user offline \n", clientfd);
    STRU_OFFLINE *request_rq = (STRU_OFFLINE*)szbuf;
    m_map_id_to_userinfo.Del(request_rq->m_userid);
}

//聊天转发
void Logic::chat_resend(int clientfd, char *szbuf, int nlen) {
    printf("client : %d user chat \n", clientfd);
    STRU_CHAT_RQ* rq = (STRU_CHAT_RQ*)szbuf;
    auto* rq_s = new STRU_CHAT_RQ(*rq);
    int user_id = rq->userid;
    int room_id = rq->roomid;
    STRU_CHAT_RS rs;
    list<int> lst;
    list<STRU_CHAT_RQ*> chats;
    //检查合法性
    if(!m_map_id_to_userinfo.Is_exist(user_id)){
        rs.result = chat_user_no_exist;
        goto l;
    }

    if(!m_map_room_to_userlist.Is_exist(room_id)){
        rs.result = chat_room_no_exist;
        goto l;
    }
    m_map_room_to_userlist.Find(room_id, lst);
    if(m_map_room_chatinfo.Is_exist(room_id)){
        m_map_room_chatinfo.Find(room_id, chats);
        chats.push_back(rq_s);
        m_map_room_chatinfo.Insert(room_id, chats);
    }
    //转发
    if(!lst.empty())
        for(auto ite = lst.begin(); ite != lst.end(); ++ite){
            int member_id = *ite;
            if(member_id == user_id) continue;
            UserInfo * user_info;
            if(m_map_id_to_userinfo.Is_exist(member_id)){
                m_map_id_to_userinfo.Find(member_id, user_info);
            }
            int sock_fd = user_info->m_sockfd;
            Send_data(sock_fd, (char*)rq, nlen);
        }
    //返回结果
    l : Send_data(clientfd, (char*)&rs, sizeof(rs));
}

void Logic::file_upload(int clientfd, char *szbuf, int nlen) {
    printf("client %d file upload\n", clientfd);
    //得到房间号
    STRU_UPLOAD_FILE_RQ *rq = (STRU_UPLOAD_FILE_RQ*)szbuf;
    int user_id = rq->userid;
    int room_id = rq->room_id;

    int64_t time_key = rq->userid*(int64_t)1e10 + rq->timestamp;
    printf("userid : %d, timestamp : %d", rq->userid, rq->timestamp);
    printf("time key is : %lld", time_key);
    FileInfo *info = nullptr;
    //创建文件信息
    STRU_UPLOAD_FILE_RS rs;
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    //查看秒传（服务器中已经存在）
    bool is_exist = false;
    for(auto ite = m_map_time_fileinfo.m.begin(); ite != m_map_time_fileinfo.m.end(); ++ite){
        if(rq->md5 == ite->second->md5){
            is_exist = true;
            info = ite->second;
        }
    }
    if(is_exist){
        rs.result = 2;
        //传完，转发给房间各成员信息
        int room_id = info->room_id;
        list<int> lst;
        m_map_room_to_userlist.Find(room_id, lst);
        for(auto ite = lst.begin(); ite != lst.end(); ++ite){
            int member_id = *ite;
            if(member_id == rq->userid) continue;
            printf("send client %d\n", member_id);
            UserInfo * user_info;
            if(m_map_id_to_userinfo.Is_exist(member_id)){
                m_map_id_to_userinfo.Find(member_id, user_info);
            }
            int sock_fd = user_info->m_sockfd;
            STRU_UPLOAD_FILE_RQ upload_rq;
            upload_rq.timestamp = rq->timestamp;
            upload_rq.userid = rq->userid;
            upload_rq.size = info->size;
            upload_rq.room_id = info->room_id;
            strcpy(upload_rq.fileName, info->name.c_str());
            strcpy(upload_rq.dir, info->dir.c_str());
            strcpy(upload_rq.md5, info->md5.c_str());
            strcpy(upload_rq.time, info->time.c_str());
            Send_data(sock_fd, (char*)&upload_rq, sizeof(upload_rq));
        }
    }else{
        info = new FileInfo;
        info->absolute_path = string(DEF_FILE_PATH) + rq->md5;
        printf("%s", info->absolute_path.c_str());
        info->md5 = rq->md5;
        info->name = rq->fileName;
        info->size = rq->size;
        info->time = rq->time;
        info->type = rq->fileType;
        info->room_id = rq->room_id;
        info->user_id = rq->userid;
        rs.result = 1; //成功
        //生成空文件 linux文件IO
        info->file_fd = open(info->absolute_path.c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0777);
        //map存储
        if(!m_map_time_fileinfo.Is_exist(time_key)){
            m_map_time_fileinfo.Insert(time_key, info);
        }
        if(info->file_fd < 0){
            printf("open file fail : %s\n", strerror(errno));
            goto l;
        }
        //存储文件信息到本地数据库
        Connection_RAII sql(m_sql_pool);
        char sql_buf[1024] = {0};
        sprintf(sql_buf, "insert into upload_files(user_id, pos, time_key) values(%d, %d, %ld)"
        , info->user_id, 0, time_key);
        if(!sql.Update_mysql(sql_buf))
            printf("file info insert mysql fail when upload\n");
        //得到file_id存储在缓存中
        memset(sql_buf, 0, sizeof(1024));
        sprintf(sql_buf, "select file_id from upload_files where time_key = %ld"
        , time_key);
        list<string> lst;
        if(!sql.Select_mysql(sql_buf, 1, lst))
            printf("upload file select file_id fail\n");
        info->file_id = stoi(lst.front());
        printf("upload file id is : %d\n", info->file_id);
    }

    l : Send_data(clientfd, (char*)&rs, sizeof(rs));
}

void Logic::file_content_rq(int clientfd, char *szbuf, int nlen) {
    printf("%d file content rq(write)\n", clientfd);
    STRU_FILE_CONTENT_RQ *rq = (STRU_FILE_CONTENT_RQ*)szbuf;
    int64_t time_key = rq->userid*(int64_t)1e10 + rq->timestamp;
    STRU_FILE_CONTENT_RS rs;
    if(m_map_time_fileinfo.Is_exist(time_key)){
        FileInfo *info = nullptr;
        m_map_time_fileinfo.Find(time_key, info);
        int len = write(info->file_fd, rq->content, rq->len);
        if(len != rq->len){//写入失败，退回
            lseek(info->file_fd, -1*len, SEEK_CUR);
            rs.len = rq->len;
            printf("write fail \n");
        }else{
            rs.result = 1;
            info->pos += len;
            rs.len = len;
            printf("rs len %d\n", rs.len);
            //每次写入都更新mysql信息
            Connection_RAII sql(m_sql_pool);
            char sql_buf[1024]{0};
            sprintf(sql_buf, "update upload_files set pos = %d where file_id = %d and user_id = %d",
                    info->pos, info->file_id, info->user_id);
            if(!sql.Update_mysql(sql_buf))
                printf("update upload file pos fail");
            //查看是否上传完
            if(info->pos == info->size){
                close(info->file_fd);
                printf("upload complete\n");
                //传完，转发给房间各成员信息
                int room_id = info->room_id;
                list<int> lst;
                m_map_room_to_userlist.Find(room_id, lst);
                for(auto ite = lst.begin(); ite != lst.end(); ++ite){
                    int member_id = *ite;
                    if(member_id == rq->userid) continue;
                    UserInfo * user_info;
                    if(m_map_id_to_userinfo.Is_exist(member_id)){
                        m_map_id_to_userinfo.Find(member_id, user_info);
                    }
                    int sock_fd = user_info->m_sockfd;
                    STRU_UPLOAD_FILE_RQ upload_rq;
                    upload_rq.timestamp = rq->timestamp;
                    upload_rq.userid = rq->userid;
                    upload_rq.size = info->size;
                    upload_rq.room_id = info->room_id;
                    strcpy(upload_rq.fileName, info->name.c_str());
                    strcpy(upload_rq.dir, info->dir.c_str());
                    strcpy(upload_rq.md5, info->md5.c_str());
                    strcpy(upload_rq.time, info->time.c_str());
                    Send_data(sock_fd, (char*)&upload_rq, sizeof(upload_rq));
                }
            }
        }
    }
    rs.timestamp = rq->timestamp;
    rs.userid = rq->userid;
    Send_data(clientfd, (char*)&rs, sizeof(rs));
}

void Logic::file_download_rq(int clientfd, char *szbuf, int nlen) {
    printf("%d file download rq\n", clientfd);
    STRU_FILE_DOWNLOAD_RQ *rq = (STRU_FILE_DOWNLOAD_RQ*)szbuf;
    int64_t time_key = rq->userid*(int64_t)1e10+rq->timestamp;
    STRU_FILE_CONTENT_RQ content_rq;
    content_rq.userid = rq->userid;
    content_rq.timestamp = rq->timestamp;
    printf("userid : %d, timestamp : %d", rq->userid, rq->timestamp);
    printf("time key is : %lld\n", time_key);
    FileInfo *info = nullptr;
    if(m_map_time_fileinfo.Is_exist(time_key)){
        m_map_time_fileinfo.Find(time_key, info);
    }else return ;
    string path = string(DEF_FILE_PATH) + info->md5;
    info->file_fd = open(path.c_str(), O_RDONLY);
    if(info->file_fd < 0) {
        printf("file open fail\n");
        return;
    }
    //存储下载信息到数据库
    Connection_RAII sql(m_sql_pool);
    char sql_buf[1024]{0};
    sprintf(sql_buf, "insert into download_files(file_id, use_id, pos, is_ok, download_address, useful)"
                     " values(%d, %d, %d, %s, %s, %s)",
            info->user_id, info->file_id, 0, "false", info->dir.c_str(), "true");
    if(!sql.Update_mysql(sql_buf))
        printf("update download file pos fail");
    //读文件块
    content_rq.len = read(info->file_fd, content_rq.content, DEF_BUFFER);

    //文件快请求 rq 为读接收写 ； rs 为写接收读
    Send_data(clientfd, (char*)&content_rq, sizeof(content_rq));
}

void Logic::file_content_rs(int clientfd, char *szbuf, int nlen) {
    printf("%d file content rs(read)\n", clientfd);
    STRU_FILE_CONTENT_RS *rs = (STRU_FILE_CONTENT_RS*)szbuf;
    int64_t time_key = rs->userid*(int64_t)1e10 + rs->timestamp;
    FileInfo *info = nullptr;
    if(m_map_time_fileinfo.Is_exist(time_key)){
        m_map_time_fileinfo.Find(time_key, info);
    }
    if(rs->result == 0){ //写失败，回退文件指针在读一次
        lseek(info->file_fd, -1*rs->len, SEEK_CUR);
        info->pos -= rs->len;
    }else {
        //写成功，更新本地数据库
        info->pos += rs->len;
        Connection_RAII sql(m_sql_pool);
        char sql_buf[1024]{0};
        sprintf(sql_buf, "update download_files set pos = %d where file_id = %d and user_id = %d",
                info->pos, info->file_id, info->user_id);
        if(!sql.Update_mysql(sql_buf))
            printf("update download file pos fail");
    }
    if(info->pos == info->size){
        //写结束，置位
        Connection_RAII sql(m_sql_pool);
        char sql_buf[1024]{0};
        sprintf(sql_buf, "update download_files set is_ok = %s where file_id = %d and use_id = %d",
                "true", info->file_id, info->user_id);
        if(!sql.Update_mysql(sql_buf))
            printf("update download file is_ok fail");
        close(info->file_fd);
        return;
    }
    STRU_FILE_CONTENT_RQ rq;
    rq.len = read(info->file_fd, rq.content, DEF_BUFFER);
    rq.userid = rs->userid;
    rq.timestamp = rs->timestamp;
    Send_data(clientfd, (char*)&rq, sizeof(rq));
}
