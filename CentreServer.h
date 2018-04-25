#ifndef NATTRAVERSECENTREMUDUO_CENTRESERVER_H
#define NATTRAVERSECENTREMUDUO_CENTRESERVER_H

#include <muduo/net/TcpServer.h>
#include <iostream>
#include <map>
#include <string>
#include <ostream>
// RFC 862

enum  EMTraverseState{
    EMTState_FAIL,
    EMTState_OK,
    EMTState_INIT,
};

class connSession {
public:
    muduo::net::TcpConnectionPtr conn;
    int natType=-1;
    std::string hostAddr;
    std::string netMask;
    std::string reflexAddr;
    EMTraverseState emTraverseState=EMTState_INIT;
    bool canStart=false;
    friend  std::ostream& operator<<(std::ostream&out,connSession& cs){
        std::cout<<cs.netMask<<"|"<<cs.hostAddr<<"|"<<cs.reflexAddr<<"|"<<cs.natType<<"|"<<cs.emTraverseState;
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
    //void onRemoveConnection(const muduo::net::TcpConnectionPtr& conn);
    void ControlClientOnMessage(const muduo::net::TcpConnectionPtr& conn,
                                     muduo::net::Buffer* buf,
                                     muduo::Timestamp time);
    void TurnTcpOnMessage(const muduo::net::TcpConnectionPtr& conn,
                               muduo::net::Buffer* buf,
                               muduo::Timestamp time);
  muduo::net::TcpServer server_;
    std::map<std::string,connSession> connMap;
    std::map<std::string,std::string>traversePairMap;

    //std::map<std::string,int>natTypeMap;

};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
