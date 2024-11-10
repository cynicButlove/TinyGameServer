cmake_minimum_required(VERSION 3.28)
project(TinyGameServer)

set(CMAKE_CXX_STANDARD 17)

# Find Protobuf package
find_package(Protobuf REQUIRED)

# Include directories for Protobuf
include_directories(${Protobuf_INCLUDE_DIRS})

# Manually specify MySQL Connector/C++ paths
# Modify these paths as needed to match your system setup
set(MYSQL_INCLUDE_DIR /usr/include/mysql_connection.h)  # MySQL Connector/C++头文件目录
set(MYSQL_LIB /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so)  # MySQL Connector库文件


# Add MySQL include directory
include_directories(${MYSQL_INCLUDE_DIR})

# Add executable and link Protobuf and MySQL libraries
add_executable(TinyGameServer
        main.cpp
        NetConnect/connect.cpp
        Protobuf/messages.pb.cc
        HandleMessages/handler.cpp
        HandleMessages/Singleton.h
        HandleMessages/Client.h
)

# Link Protobuf and MySQL libraries
target_link_libraries(TinyGameServer ${Protobuf_LIBRARIES} ${MYSQL_LIB})
