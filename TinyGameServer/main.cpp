#include <iostream>
#include "NetConnect/connect.cpp"

using namespace TinyGameServer;

int main() {
    Handler::Instance().CreateHandler();
    NetConnect netConnect;
    netConnect.SetConnect();
    return 0;
}
