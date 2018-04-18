#ifndef MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
#define MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H

#include <muduo/net/TcpServer.h>
#include <iostream>
#include "map"
#include "string"
#include "ostream"
// RFC 862
class connSession {
public:
    muduo::net::TcpConnectionPtr conn;
    int natType=-1;
    std::string hostAddr;
    std::string netMask;
    std::string reflexAddr;
    bool canTraverse=false;
    friend  std::ostream& operator<<(std::ostream&out,connSession& cs){
        std::cout<<cs.netMask<<"|"<<cs.hostAddr<<"|"<<cs.reflexAddr<<"|"<<cs.natType<<"|"<<cs.canTraverse;
        return out;
    }
};
class CentreServer
{
 public:
  CentreServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr);

  void start();  // calls server_.start();

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp time);
    void onRemoveConnection(const muduo::net::TcpConnectionPtr& conn);
  muduo::net::TcpServer server_;
    std::map<std::string,connSession> connMap;
    std::map<std::string,std::string>natTraverseMap;

    //std::map<std::string,int>natTypeMap;

};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
