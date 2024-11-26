cmake_minimum_required(VERSION 3.28)
project(TinyGameServer)

set(CMAKE_CXX_STANDARD 17)

# Find Protobuf package
find_package(Protobuf REQUIRED)

# Find TBB package
find_package(TBB REQUIRED)

# Include directories for Protobuf and TBB
include_directories(${Protobuf_INCLUDE_DIRS} ${TBB_INCLUDE_DIRS})

# Manually specify MySQL Connector/C++ paths
# Modify these paths as needed to match your system setup
set(MYSQL_INCLUDE_DIR /usr/include/mysql_connection.h)  # MySQL Connector/C++ header directory
set(MYSQL_LIB /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so)  # MySQL Connector library file

# Add MySQL include directory
include_directories(${MYSQL_INCLUDE_DIR})

# Add executable and link Protobuf, MySQL, and TBB libraries
add_executable(TinyGameServer
        main.cpp
        NetConnect/connect.cpp
        Protobuf/messages.pb.cc
        HandleMessages/handler.cpp
        HandleMessages/Singleton.h
        HandleMessages/Client.h
)

# Link Protobuf, MySQL, and TBB libraries
target_link_libraries(TinyGameServer ${Protobuf_LIBRARIES} ${MYSQL_LIB} ${TBB_LIBRARIES} TBB::tbb)