//
// Created by zhangshiping on 24-11-5.
//


#include <sys/socket.h>
#include <queue>
#include <condition_variable>
#include "../Protobuf/messages.pb.h"
#include "Singleton.h"
#include <thread>
namespace TinyGameServer{
    class Handler: public Singleton<Handler>
    {

    private:
        std::queue<FullMessage> messageQueue;
        std::mutex queueMutex;
        std::condition_variable queueCondition;

    public:
        void CreateHandler() {
            std::thread workerThread1(&Handler::WorkerThread, this);
            std::thread workerThread2(&Handler::WorkerThread, this);
            workerThread1.detach();
            workerThread2.detach();
        }

        void HandleClientMessage(int client_fd) {
            char buffer[1024];
            ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

            if (bytes_received > 0) {
                FullMessage msg;
                if (msg.ParseFromArray(buffer, bytes_received)) {
                    // 将消息放入队列
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        messageQueue.push(msg);
                    }
                    queueCondition.notify_one(); // 通知工作线程有新任务
                }
            } else {
                // 处理接收错误
                if (bytes_received == 0) {
                    // 客户端关闭连接
                    std::cout << "Client closed connection" << std::endl;
                } else {
                    // 出错
                    perror("receiver error");
                }
            }
        }

        void WorkerThread() {
            while (true) {
                FullMessage msg;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait(lock, [&] { return !messageQueue.empty(); }); // 等待新任务
                    msg = messageQueue.front();
                    messageQueue.pop();
                }

                // 根据消息类型进行处理
                switch (msg.header().type()) {
                    case MessageType::PLAYER_POSITION:
                        //HandlePlayerPosition(msg.position());
                        break;
                    case MessageType::PLAYER_ACTION:
                        //HandlePlayerAction(msg.action());
                        break;
                    default:
                        // 处理未知类型
                        break;
                }
            }
        }



    };
}