cmake_minimum_required(VERSION 3.22)
project(Meeting_Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(Meeting_Server main.cpp server/server.cpp server/server.h lock/lock.h sql/sql_connect_pool.cpp sql/sql_connect_pool.h thread_pool/thread_pool.cpp thread_pool/thread_pool.h config.h epoll_net/epoll_net.cpp epoll_net/epoll_net.h lock/lock_map.h logic/logic.cpp logic/logic.h config.h server/server.cpp server/server.h main.cpp)

target_link_libraries(Meeting_Server -lpthread -lmysqlclient)