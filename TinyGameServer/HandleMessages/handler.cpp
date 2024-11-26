//
// Created by zhangshiping on 24-11-5.
//


#include <sys/socket.h>
#include <queue>
#include "../Protobuf/messages.pb.h"
#include "Singleton.h"
#include <thread>
#include <netinet/in.h>
#include "DB.cpp"
#include "Client.h"
#include <tbb/concurrent_queue.h>


namespace TinyGameServer {

    class Task {
    public:
        int socket_fd;
        FullMessage msg;
        Task(int fd, FullMessage message) : socket_fd(fd), msg(message) {}
        Task() = default;
    };

    class RankListItem;

    class Handler : public Singleton<Handler> {
    private:
        tbb::concurrent_queue<Task> messageQueue;
        // key: socket_fd, value: Client
        std::unordered_map<int, Client> clientMap;
        // rank list
        std::vector<RankListItem> rankList;

    public:
        void CreateHandler() {
            std::thread workerThread1(&Handler::WorkerThread, this);
            workerThread1.detach();
            // 创建两个测试客户端
            Client client1 = Client(111, "RenJi_1", PlayerPosition(10, 0, 10, 0, 0, 0));
            Client client2 = Client(112, "RenJi_2", PlayerPosition(-10, 0, -10, 0, 0, 0));
            clientMap.insert(std::make_pair(-1, client1));
            clientMap.insert(std::make_pair(-2, client2));
            //
            rankList=DB_Mgr::Instance().getRankList();
        }

        void HandleClientMessage(int client_fd) {
            // 1. 读取消息长度
            uint32_t length;
            ssize_t received = recv(client_fd, &length, sizeof(length), MSG_WAITALL);
            if (received <= 0) {
                close(client_fd);
                std::cout << "close ,fd = " << client_fd << std::endl;
                clientMap.erase(client_fd);
                return;
            }
            length = ntohl(length);  // 转换为主机字节序
            // 2. 读取消息内容
            std::vector<char> buffer(length);
            received = recv(client_fd, buffer.data(), length, MSG_WAITALL);

            if (received <= 0) {
                if (received == -1) {
                    std::cerr << "recv failed, errno: " << errno << " (" << strerror(errno) << ")\n";
                    close(client_fd);
                    std::cout << "close ,fd = " << client_fd << std::endl;
                    return;
                }
            }

            // 3. 反序列化Protobuf消息
            FullMessage message;
            if (message.ParseFromArray(buffer.data(), buffer.size())) {
                // 将消息放入队列
                Task task = Task(client_fd, message);
                messageQueue.push(task);
                std::cout<<"queue unsafe size : "<< messageQueue.unsafe_size()<<std::endl;
                //std::cout<<"push fd = "<<client_fd<<" , type=  "<<(MessageType)message.header().type()<<  std::endl;
            } else {
                std::cerr << "Failed to parse message.\n";
            }
        }

        void WorkerThread() {
            while (true) {
                Task task;
                if (messageQueue.try_pop(task)) {
                    std::cout<<messageQueue.unsafe_size()<<std::endl;
                    FullMessage msg = task.msg;
                    int fd = task.socket_fd;
                    //std::cout<<"working , fd = "<<fd<<" , type=  "<<(MessageType)msg.header().type()<<  std::endl;
                    // 根据消息类型进行处理
                    switch (msg.header().type()) {
                        case MessageType::LOGIN_IN_REQ: {
                            auto login_Req = msg.login_req();
                            handleLoginReq(fd, login_Req);
                            break;
                        }
                        case MessageType::PLAYER_State:{
                            auto client=msg.player_state().client();
                            clientMap[fd].position=PlayerPosition::ToPos(client.position());
                            BroadcastMsg(fd,msg);
                            break;
                        }
                        case MessageType::GunInfo:{
                            auto gun=msg.gun_info().gun_name();
                            if(msg.gun_info().throw_()==1) gun="None";
                            clientMap[fd].gun=gun;
                            BroadcastMsg(fd,msg);
                            break;
                        }
                        case MessageType::BulletHit:{
                            auto health=msg.bullet_hit().health();
                            clientMap[fd].health=health;
                            BroadcastMsg(fd,msg);
                            break;
                        }
                        case MessageType::GunFire:
                        case MessageType::AnimatorParam:
                        case MessageType::ReloadBullet: {
                            BroadcastMsg(fd, msg);
                            break;
                        }
                        case MessageType::Logout: {
                            std::cout << "Client logout, fd: " << fd << std::endl;
                            close(fd);
                            clientMap.erase(fd);
                            break;
                        }
                        case MessageType::RankScore:{
                            UpdateRankScore(fd,msg);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }




        void handleLoginReq(int fd, const LoginInRequest &req) {
            const std::string username = req.username();
            const std::string password = req.password();
            int client_id = -1;
            auto ret=DB_Mgr::Instance().verifyRegisterUser(username, password, client_id);
            if ( ret == LoginStatus::SUCCESS) {
                // 加载其他玩家信息
                LoadOtherPlayers(fd);
                // 创建玩家角色
                auto client=Client(client_id,username,PlayerPosition());
                sendLoginRspMsg(fd ,client, LoginStatus::SUCCESS);
                // 向其他玩家广播新客户端登录消息
                BroadcastWhenNewClient(fd,client);
                // 存储到map
                clientMap.insert(std::make_pair(fd,client));
                // rank list
                UpdateRankScore(fd);
            }
            else if(ret == LoginStatus::NEW_USER){
                LoadOtherPlayers(fd);
                auto client=Client(client_id,username,PlayerPosition());
                sendLoginRspMsg(fd, client, LoginStatus::NEW_USER);
                BroadcastWhenNewClient(fd,client);
                clientMap.insert(std::make_pair(fd,client));
                UpdateRankScore(fd);
            }
            else if(ret == LoginStatus::PASSWORD_INCORRECT){
                auto client=Client(client_id,username,PlayerPosition());
                sendLoginRspMsg(fd ,client, LoginStatus::PASSWORD_INCORRECT);
            }
            else if(ret == LoginStatus::SQL_ERROR){
                auto client=Client(client_id,username,PlayerPosition());
                sendLoginRspMsg(fd, client, LoginStatus::SQL_ERROR);
            }
        }
        // 发送登录响应消息 ,创建玩家角色
        void sendLoginRspMsg(int fd, const Client& client, LoginStatus status){
            auto clientMsg=client.ToMessage();
            auto loginRsp=new LoginInResponse();
            loginRsp->set_allocated_client(&clientMsg);
            loginRsp->set_error_no((int)status);
            auto* header=new  MessageHeader();
            header->set_type(MessageType::LOGIN_IN_RSP);
            auto* msg=new FullMessage();
            msg->set_allocated_header(header);
            msg->set_allocated_login_rsp(loginRsp);
            SendToClient(fd,*msg);
        }

        // 发送消息给客户端
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
            //std::cout<<"send to fd: "<<fd<<" , type=  "<<(MessageType)msg.header().type()<<  std::endl;
            if (bytes_sent == -1) {
                std::cerr << "Failed to send message to client, errno: " << errno << " (" << strerror(errno) << ")\n";
                close(fd);
            } else {
//                std::cout << "Message sent to client, bytes: " << bytes_sent << "\n";
            }
        }


        // 向其他玩家广播新客户端登录消息
        void BroadcastWhenNewClient(int fd,Client client){
            for (const auto &pair : clientMap) {
                if(pair.first <0 ) continue;
                if(fd==pair.first) continue;
                auto clientMsg=client.ToMessage();
                auto playerLoginMsg=new PlayerLoginMsg();
                playerLoginMsg->set_allocated_client(&clientMsg);
                auto header=new MessageHeader();
                header->set_type(MessageType::PLAYER_LOGIN);
                auto msg= new FullMessage();
                msg->set_allocated_header(header);
                msg->set_allocated_player_login(playerLoginMsg);
                SendToClient(pair.first,*msg);
            }
        }
        // 加载其他玩家信息
        void LoadOtherPlayers(int fd){
            auto others = new LoadOtherPlayersMsg();
            for (const auto &pair : clientMap) {
                auto clientMsg=pair.second.ToMessage();
                // 将其他客户信息添加到消息中
                others->add_otherclients()->CopyFrom(clientMsg);
            }
            auto msg= new FullMessage();
            auto header=new MessageHeader();
            header->set_type(MessageType::LoadOtherPlayers);
            msg->set_allocated_header(header);
            msg->set_allocated_load_other_players(others);
            SendToClient(fd, *msg);
        }
        // 向其他玩家同步玩家信息
        void BroadcastMsg(int fd ,const FullMessage& msg,bool includeSelf=false){
            for(const auto& pair:clientMap){
                if(pair.first <0) continue;
                if(fd==pair.first && !includeSelf) continue;
                SendToClient(pair.first,msg);
            }
        }


        void UpdateRankScore(int fd, FullMessage message) {
            auto rankScore = message.rank_score();
            int client_id = rankScore.client_id();
            int score = rankScore.score();
            if(rankList.size()==0){
                rankList.push_back(RankListItem(client_id,score));
            }
            else{
                bool found=false;
                for(auto& item:rankList){
                    if(item.client_id==client_id){
                        item.score=std::max(item.score,score);
                        found=true;
                        break;
                    }
                }
                if(!found){
                    rankList.push_back(RankListItem(client_id,score));
                }
            }
            std::sort(rankList.begin(),rankList.end(),[](const RankListItem& a,const RankListItem& b){
                return a.score>b.score;
            });
            DB_Mgr::Instance().updateRankScore(client_id,score);
            if(rankList.size()>3){
                rankList.erase(rankList.begin()+3,rankList.end());
            }
            auto rankListMsg=new RankListMsg();
            for(const auto& item:rankList){
                auto rankListItem=new RankScoreMsg();
                rankListItem->set_client_id(item.client_id);
                rankListItem->set_score(item.score);
                rankListMsg->add_ranklist()->CopyFrom(*rankListItem);
            }
            auto msg=new FullMessage();
            auto header=new MessageHeader();
            header->set_type(MessageType::RankList);
            msg->set_allocated_header(header);
            msg->set_allocated_rank_list(rankListMsg);
            BroadcastMsg(fd, *msg, true);
        }

        void  UpdateRankScore(int fd){
            auto rankListMsg=new RankListMsg();
            for(const auto& item:rankList){
                auto rankListItem=new RankScoreMsg();
                rankListItem->set_client_id(item.client_id);
                rankListItem->set_score(item.score);
                rankListMsg->add_ranklist()->CopyFrom(*rankListItem);
            }
            auto msg=new FullMessage();
            auto header=new MessageHeader();
            header->set_type(MessageType::RankList);
            msg->set_allocated_header(header);
            msg->set_allocated_rank_list(rankListMsg);
            BroadcastMsg(fd, *msg, true);
        }







    };
}