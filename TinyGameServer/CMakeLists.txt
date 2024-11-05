cmake_minimum_required(VERSION 3.28)
project(TinyGameServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(TinyGameServer main.cpp
        NetConnect/connect.cpp
        Protobuf/messages.pb.cc
        HandleMessages/handler.cpp
        HandleMessages/Singleton.h)
