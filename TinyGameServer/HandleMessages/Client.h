//
// Created by zhangshiping on 24-11-10.
//

#ifndef TINYGAMESERVER_CLIENT_H
#define TINYGAMESERVER_CLIENT_H


#include <string>
#include "../Protobuf/messages.pb.h"

namespace TinyGameServer {
    class PlayerPosition{
    public:
        float p_x ;
        float p_y ;
        float p_z ;
        float rotation_x ;
        float rotation_y = 180;
        float rotation_z ;
        PlayerPosition(){
            p_x=0;
            p_x=0;
            p_z=0;
            rotation_x=0;
            rotation_y=180;
            rotation_z=0;
        }
        PlayerPosition(float x,float y,float z,float r_x,float r_y,float r_z){
            p_x=x;
            p_x=y;
            p_z=z;
            rotation_x=r_x;
            rotation_y=180;
            rotation_z=r_z;

        }
    };
    enum class MoveState
    {
        idle,
        Walking,
        Running,
        Crouching,
        Jumping,
        Dropping

    };


    class Client {
    public:
        int client_id ;
        std::string username ;
        MoveState state ;
        PlayerPosition position ;
        Client(int id,std::string name,PlayerPosition pos){
            client_id=id;
            username=name;
            state=MoveState::idle;
            position=pos;
        }
        ClientMsg ToMessage(){
            ClientMsg msg;
            msg.set_client_id( client_id);
            msg.set_username(username);
            msg.set_state((int)state);
            PlayerPositionMsg playerPositionMsg;
            playerPositionMsg.set_x(position.p_x);
            playerPositionMsg.set_y(position.p_y);
            playerPositionMsg.set_z(position.p_z);
            playerPositionMsg.set_rotation_x(position.rotation_x);
            playerPositionMsg.set_rotation_y(position.rotation_y);
            playerPositionMsg.set_rotation_z(position.rotation_z);
            msg.set_allocated_position(&playerPositionMsg);
            return msg;
        }
    };
}





#endif //TINYGAMESERVER_CLIENT_H
