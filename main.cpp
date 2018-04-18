#include "CentreServer.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <unistd.h>
//#include<pthread.h>
// using namespace muduo;
// using namespace muduo::net;

int main()
{
    //LOG_INFO << "pid = " << getpid();
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr("192.168.1.103",2007);
    CentreServer centreServer(&loop, listenAddr);
    centreServer.start();



    loop.loop();
}
