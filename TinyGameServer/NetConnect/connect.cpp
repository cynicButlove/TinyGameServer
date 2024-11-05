//
// Created by zhangshiping on 24-11-3.
//
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../Protobuf/messages.pb.h"
#include "../HandleMessages/handler.cpp"
namespace TinyGameServer{
    class NetConnect {

    private:
        const int MAX_EVENTS = 1024;
        const int PORT = 8080;


        int setNonBlocking(int fd) {
            int flags = fcntl(fd, F_GETFL, 0);
            return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }

    public:

        int SetConnect(){
            int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (listen_fd == -1) {
                perror("socket");
                return -1;
            }

            // 设置为非阻塞模式
            setNonBlocking(listen_fd);

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(PORT);

            if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
                perror("bind");
                close(listen_fd);
                return -1;
            }

            if (listen(listen_fd, SOMAXCONN) == -1) {
                perror("listen");
                close(listen_fd);
                return -1;
            }

            // 创建epoll实例
            int epoll_fd = epoll_create1(0);
            if (epoll_fd == -1) {
                perror("epoll_create1");
                close(listen_fd);
                return -1;
            }

            epoll_event ev{};
            ev.events = EPOLLIN;
            ev.data.fd = listen_fd;

            // 添加监听socket到epoll中
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
                perror("epoll_ctl");
                close(listen_fd);
                close(epoll_fd);
                return -1;
            }

            epoll_event events[MAX_EVENTS];

            while (true) {
                int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
                for (int i = 0; i < nfds; ++i) {
                    if (events[i].data.fd == listen_fd) {
                        // 有新的连接
                        sockaddr_in client_addr{};
                        socklen_t client_len = sizeof(client_addr);
                        int client_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_len);
                        if (client_fd == -1) {
                            perror("accept");
                            continue;
                        }
                        setNonBlocking(client_fd);
                        // 添加客户端socket到epoll中
                        ev.events = EPOLLIN | EPOLLET; // 边缘触发
                        ev.data.fd = client_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                            perror("epoll_ctl: client_fd");
                            close(client_fd);
                            continue;
                        }
                    } else {
                        // 处理客户端消息
                        if (events[i].events & EPOLLIN) {
                            Handler::Instance().HandleClientMessage(events[i].data.fd);
                        }
                    }
                }
            }

            close(listen_fd);
            close(epoll_fd);
            return 0;
        }




    };
}
