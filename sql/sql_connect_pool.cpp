//
// Created by shotacon on 23-1-27.
//

#include "sql_connect_pool.h"
#include "stdexcept"

using std::logic_error;

void Sql_Connection_Pool::Destroy_pool() {
    m_lock.lock();

    if(m_conn_list.size() > 0){
        for(auto it : m_conn_list){
            mysql_close(it);
        }
        m_free_conn_num = 0;
        m_max_conn_num = 0;
        m_conn_list.clear();
    }
    m_lock.unlock();
}

void Sql_Connection_Pool::init(string url, unsigned port, string user, string password, string database_name, int max_conn) {
    m_url = url;
    m_port = port;
    m_user = user;
    m_password = password;
    m_database_name = database_name;

    for (int i = 0; i < max_conn; ++i) {
        MYSQL * conn = nullptr;
        conn = mysql_init(conn);
        if(!conn)
            throw logic_error("sql_connect_pool >> init >> mysql_init fail");

        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port,
                                  nullptr, 0);
        if(!conn)
            throw logic_error("sql_connect_pool >> init >> mysql_real_connect fail");
        m_conn_list.push_back(conn);
        ++m_free_conn_num;
    }
    m_sem = sem(m_free_conn_num);
    m_max_conn_num = m_free_conn_num;
}

MYSQL *Sql_Connection_Pool::Get_sql_connect() {
    MYSQL * conn = nullptr;
    if(m_conn_list.size() == 0){
        return nullptr;
    }
    m_sem.wait();//原子操作，sem_num -1 线程占用
    m_lock.lock();

    conn = m_conn_list.front();
    m_conn_list.pop_front();

    --m_free_conn_num;
    ++m_cur_conn_num;

    m_lock.unlock();
    if(!conn) throw logic_error("get connection mysql conn is null");
    return conn;
}

bool Sql_Connection_Pool::Realse_sql_connect(MYSQL *sql_conn) {
    if(!sql_conn)
        return false;
    m_lock.lock();
    m_conn_list.push_back(sql_conn);
    ++m_free_conn_num;
    --m_cur_conn_num;

    m_lock.unlock();
    m_sem.post();//sem_num ++ 释放资源
    return false;
}

Sql_Connection_Pool *Sql_Connection_Pool::Get_instance() {
    static Sql_Connection_Pool sql_conn_pool;
    return &sql_conn_pool;
}

Sql_Connection_Pool::Sql_Connection_Pool() {
    m_free_conn_num = 0;
    m_max_conn_num = 0;
}

Sql_Connection_Pool::~Sql_Connection_Pool() {
    Destroy_pool();
}


Connection_RAII::Connection_RAII(Sql_Connection_Pool *conn_pool) {
    m_sql_conn_pool = conn_pool;
    m_conn = m_sql_conn_pool->Get_sql_connect();
}

Connection_RAII::~Connection_RAII() {
    m_sql_conn_pool->Realse_sql_connect(m_conn);
}

bool Connection_RAII::Select_mysql(char *sql, int column, list<string> &lst) {
    if(mysql_query(m_conn, sql) != 0) return false;
    MYSQL_RES* res = mysql_store_result(m_conn);
    if(!res) return false;

    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        for (int i = 0; i < column; ++i) {
            lst.emplace_back(row[i]);
        }
    }
    return true;
}

bool Connection_RAII::Update_mysql(char *sql) {
    if(!sql) return false;

    if(mysql_query(m_conn, sql) != 0) return false;

    return true;
}

