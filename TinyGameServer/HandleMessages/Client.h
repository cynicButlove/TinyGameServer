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
        float gun_rotation_x ;
        float gun_rotation_y ;
        float gun_rotation_z ;
        PlayerPosition(){
            p_x=0;
            p_y=0;
            p_z=0;
            rotation_x=0;
            rotation_y=180;
            rotation_z=0;
            gun_rotation_x=0;
            gun_rotation_y=0;
            gun_rotation_z=0;
        }
        PlayerPosition(float x,float y,float z,float r_x,float r_y,float r_z){
            p_x=x;
            p_y=y>0?y:0;
            p_z=z;
            rotation_x=r_x;
            rotation_y=180;
            rotation_z=r_z;
            gun_rotation_x=0;
            gun_rotation_y=0;
            gun_rotation_z=0;
        }

        static PlayerPosition ToPos(const PlayerPositionMsg& msg){
            PlayerPosition pos;
            pos.p_x=msg.x();
            pos.p_y=msg.y();
            pos.p_z=msg.z();
            pos.rotation_x=msg.rotation_x();
            pos.rotation_y=msg.rotation_y();
            pos.rotation_z=msg.rotation_z();
            pos.gun_rotation_x=msg.gun_rotation_x();
            pos.gun_rotation_y=msg.gun_rotation_y();
            pos.gun_rotation_z=msg.gun_rotation_z();
            return pos;
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
        int health ;
        std::string gun;

        Client()=default;

        Client(int id,std::string name,PlayerPosition pos){
            client_id=id;
            username=name;
            state=MoveState::idle;
            position=pos;
            health=100;
            gun="None";
        }
        ClientMsg ToMessage() const{
            auto* msg= new ClientMsg();
            msg->set_client_id( client_id);
            msg->set_username(username);
            msg->set_state((int)state);
            auto playerPositionMsg=new PlayerPositionMsg();
            playerPositionMsg->set_x(position.p_x);
            playerPositionMsg->set_y(position.p_y);
            playerPositionMsg->set_z(position.p_z);
            playerPositionMsg->set_rotation_x(position.rotation_x);
            playerPositionMsg->set_rotation_y(position.rotation_y);
            playerPositionMsg->set_rotation_z(position.rotation_z);
            playerPositionMsg->set_gun_rotation_x(position.gun_rotation_x);
            playerPositionMsg->set_gun_rotation_y(position.gun_rotation_y);
            playerPositionMsg->set_gun_rotation_z(position.gun_rotation_z);
            msg->set_allocated_position(playerPositionMsg);
            msg->set_health(health);
            msg->set_gun_name(gun);
            return *msg;
        }
    };
}





#endif //TINYGAMESERVER_CLIENT_H
