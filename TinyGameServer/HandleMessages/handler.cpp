//
// Created by zhangshiping on 24-11-5.
//


#include <sys/socket.h>
#include <queue>
#include <condition_variable>
#include "../Protobuf/messages.pb.h"
#include "Singleton.h"
#include <thread>
#include <netinet/in.h>
#include "DB.cpp"
#include "Client.h"

namespace TinyGameServer{
    class Task{
    public:
        int socket_fd;
        FullMessage msg;
        Task(int fd,FullMessage message){
            socket_fd=fd;
            msg=message;
        }
        Task(){}
    };

    class Handler: public Singleton<Handler>
    {

    private:
        std::queue<Task> messageQueue;
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
            // 1. 读取消息长度
            uint32_t length;
            ssize_t received = recv(client_fd, &length, sizeof(length), MSG_WAITALL);
            if (received <= 0) {
                close(client_fd);
                std::cout<<"close ,fd = "<<client_fd<<  std::endl;
                return;
            }
            length = ntohl(length);  // 转换为主机字节序
            std::cout<<"length: "<<length<<std::endl;
            // 2. 读取消息内容
            std::vector<char> buffer(length);
            received = recv(client_fd, buffer.data(), length, MSG_WAITALL);
            std::cout<<"received: "<<received<<std::endl;

            if (received <= 0) {
                // 错误处理示例
                if (received == -1) {
                    std::cerr << "recv failed, errno: " << errno << " (" << strerror(errno) << ")\n";
                    close(client_fd);
                    std::cout<<"close ,fd = "<<client_fd<<  std::endl;
                    return;
                }
            }

            // 3. 反序列化Protobuf消息
            FullMessage message;
            if (message.ParseFromArray(buffer.data(), buffer.size())) {
                // 将消息放入队列
                {
                    Task task=Task(client_fd,message);
                    std::cout<<"message: "<<message.header().type()<<std::endl;
                    std::lock_guard<std::mutex> lock(queueMutex);
                    messageQueue.push(task);
                }
                queueCondition.notify_one(); // 通知工作线程有新任务
            } else {
                std::cerr << "Failed to parse message.\n";
            }
        }

        void WorkerThread() {
            while (true) {
                Task task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait(lock, [&] { return !messageQueue.empty(); }); // 等待新任务
                    task = messageQueue.front();
                    messageQueue.pop();
                }
                FullMessage msg=task.msg;
                int fd=task.socket_fd;
                // 根据消息类型进行处理
                switch (msg.header().type()) {
                    case MessageType::LOGIN_IN_REQ: {
                        auto login_Req = msg.login_req();
                        std::cout << "Received login request. Username: " << login_Req.username()
                                  << ", Password: " << login_Req.password() << std::endl;
                        // 处理登录请求
                        handleLoginReq(fd,login_Req);
                        break;
                    }
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





        void handleLoginReq(int fd, const LoginInRequest &req) {
            const std::string username = req.username();
            const std::string password = req.password();
            int client_id = -1;
            if (DB_Mgr::Instance().verifyRegisterUser(username, password, client_id) == LoginStatus::SUCCESS) {
                auto client=Client(client_id,username,PlayerPosition());
                FullMessage msg= setLoginRspMsg(client,LoginStatus::SUCCESS);
                SendToClient(fd,msg);
            }
            else if(DB_Mgr::Instance().verifyRegisterUser(username, password, client_id) == LoginStatus::PASSWORD_INCORRECT){
                auto client=Client(client_id,username,PlayerPosition());
                FullMessage msg= setLoginRspMsg(client,LoginStatus::PASSWORD_INCORRECT);
                SendToClient(fd,msg);
            }
            else if(DB_Mgr::Instance().verifyRegisterUser(username, password, client_id) == LoginStatus::NEW_USER){
                auto client=Client(client_id,username,PlayerPosition());
                FullMessage msg= setLoginRspMsg(client,LoginStatus::NEW_USER);
                SendToClient(fd,msg);
            }
            else{
                auto client=Client(client_id,username,PlayerPosition());
                FullMessage msg= setLoginRspMsg(client,LoginStatus::SQL_ERROR);
                SendToClient(fd,msg);
            }
        }

        FullMessage setLoginRspMsg(Client client,LoginStatus status){
            auto clientMsg=client.ToMessage();
            auto loginRsp= LoginInResponse();
            loginRsp.set_allocated_client(&clientMsg);
            loginRsp.set_error_no((int)status);
            MessageHeader header=MessageHeader();
            header.set_type(MessageType::LOGIN_IN_RSP);
            FullMessage msg= FullMessage();
            msg.set_allocated_header(&header);
            msg.set_allocated_login_rsp(&loginRsp);
            return msg;
        }

        void SendToClient(int fd, const FullMessage& msg) {
            // 1. 将消息序列化为字节数组
            std::string serialized_data;
            if (!msg.SerializeToString(&serialized_data)) {
                std::cerr << "Failed to serialize message.\n";
                return;
            }

            // 2. 获取消息长度并转换为网络字节序（大端序）
            uint32_t data_length = htonl(serialized_data.size());

            // 3. 构建包含长度前缀和消息内容的完整数据包
            std::vector<char> packet(sizeof(data_length) + serialized_data.size());
            memcpy(packet.data(), &data_length, sizeof(data_length));  // 写入长度前缀
            memcpy(packet.data() + sizeof(data_length), serialized_data.data(), serialized_data.size());  // 写入消息内容

            // 4. 一次性发送完整数据包
            ssize_t bytes_sent = send(fd, packet.data(), packet.size(), 0);
            if (bytes_sent == -1) {
                std::cerr << "Failed to send message to client, errno: " << errno << " (" << strerror(errno) << ")\n";
                close(fd);
            } else {
                std::cout << "Message sent to client, bytes: " << bytes_sent << "\n";
            }
        }

    };
}