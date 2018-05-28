#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "CentreServer.h"
#include "TcpTurnServer.h"
#include "UdpTurnServer.hpp"
#include <unistd.h>
#include <boost/bind/placeholders.hpp>
#include <arpa/inet.h>
#include <signal.h>

//#include<pthread.h>
// using namespace muduo;
// using namespace muduo::net;


#include "UdpTurnServer.hpp"


//int add(int a,int b){
//    return a+b;
//}
//int add(int &a,int& b){
//    return a+b;
//}
int main()
{
//    int a=3,b=4;
//    cout<<add(a,b)<<endl;
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


    //int netmask=0xffffffff,uIP=0xc0a80193,pIP=0xc0a80165;
    //cout<<netmask&
//    uint32_t  uiNetMask=ntohl(inet_addr("255.255.255.0"));
//    uint32_t  uiUNameIP=ntohl(inet_addr("192.168.1.147"));
//    uint32_t  uiPeerNameIP=ntohl(inet_addr("192.168.1.101"));
//    printf("%x\n",uiNetMask);
//    char buf[]="name=1234";
//    char str1[32]={0};
//    char str2[32]={0};
//    sscanf(buf,"name=%s",str2);
//    printf("%s\n",str2);
    //printf("\n");
    //printf("sdfs\n");
//    while(1);
    //return 1;
    /***************************************************/

    //signal(SIGURG,SIG_IGN);

    string s="12345";
    char chs[20]={1,2,3,4,5,6,6,77,8};
    strncpy(chs,s.c_str(),s.size());
    pthread_t ptd;
    int udpPort=UDP_TURN_PORT;
    pthread_create(&ptd,NULL,turnUDP,&udpPort);
    //while(1);

    //LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;

    CentreServer centreServer(&loop, muduo::net::InetAddress (2007));
    centreServer.start();


    TcpTurnServer tcpTurnServer(&loop, muduo::net::InetAddress (TCP_TURN_PORT));
    tcpTurnServer.start();


    loop.loop();
}
