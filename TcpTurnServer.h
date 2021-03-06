#ifndef NATTRAVERSECENTREMUDUO_TCPTURNSERVER_H
#define NATTRAVERSECENTREMUDUO_TCPTURNSERVER_H


#include <muduo/net/TcpServer.h>

// RFC 862
class TcpTurnServer
{
 public:
  TcpTurnServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr);

  void start();  // calls server_.start();

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp time);

  muduo::net::TcpServer server_;
    std::map<std::string, std::pair< std::string,std::string> > idMap;
    std::map<std::string,std::string> turnMap;
    std::map<std::string, muduo::net::TcpConnectionPtr >addrToConnMap;
};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
