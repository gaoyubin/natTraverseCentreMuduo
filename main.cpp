#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "CentreServer.h"
#include "TcpTurnServer.h"
#include "UdpTurnServer.hpp"
#include <unistd.h>
#include <boost/bind/placeholders.hpp>
#include <arpa/inet.h>

//#include<pthread.h>
// using namespace muduo;
// using namespace muduo::net;


#include "UdpTurnServer.hpp"

int main()
{
//    std::map<int ,std::string>m;
//    std::cout<<m[2]<<std::endl;
//    m[4]="2sdfs";
//    m[4]="sdfsf";
//    std::cout<<m[4]<<std::endl;
//    std::cout<<m.size()<<std::endl;
    //test how many threads can generate

//    uint32_t uiNetMask=inet_addr("255.255.255.0");
//
//    uint32_t  uiUNameIP=inet_addr("192.168.1.102");
//    uint32_t  uiPeerNameIP=inet_addr("192.168.1.22");
//
//    uint32_t  a=uiNetMask & uiUNameIP;
//    uint32_t  b=uiNetMask&uiUNameIP;
//    std::cout<<a<<std::endl;
//    std::cout<<b<<std::endl;
//
//    while(1);





    pthread_t ptd;

    int udpPort=UDP_TURN_PORT;
    pthread_create(&ptd,NULL,turnUDP,&udpPort);
    //while(1);

    //LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;

    CentreServer centreServer(&loop, muduo::net::InetAddress (2007));
    centreServer.start();


    TcpTurnServer tcpTurnServer(&loop, muduo::net::InetAddress (8888));
    tcpTurnServer.start();


    loop.loop();
}
