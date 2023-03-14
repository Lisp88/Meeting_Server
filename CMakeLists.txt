cmake_minimum_required(VERSION 3.22)
project(YY_Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(YY_Server main.cpp server.cpp server.h log/log.h Sql/sql_connect_pool.cpp Sql/sql_connect_pool.h)

target_link_libraries(YY_Server -lpthread -lmysqlclient)
