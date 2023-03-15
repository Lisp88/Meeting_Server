cmake_minimum_required(VERSION 3.22)
project(YY_Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(YY_Server main.cpp server.cpp server.h lock/lock.h sql/sql_connect_pool.cpp sql/sql_connect_pool.h thread_pool/thread_pool.cpp thread_pool/thread_pool.h config.h)

target_link_libraries(YY_Server -lpthread -lmysqlclient)
