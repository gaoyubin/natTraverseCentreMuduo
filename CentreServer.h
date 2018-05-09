#ifndef NATTRAVERSECENTREMUDUO_CENTRESERVER_H
#define NATTRAVERSECENTREMUDUO_CENTRESERVER_H

#include <muduo/net/TcpServer.h>
#include <iostream>
#include <map>
#include <string>
#include <ostream>
#include <vector>
using namespace std;
// RFC 862

enum  EMTraverseState{
    EMTState_FAIL,
    EMTState_OK,
    EMTState_INIT,
};
struct Addr{
    Addr(string ip="0.0.0.0",int port=0):ip(ip),port(port){
        //cout<<"Addr constr"<<endl;
    };


    bool operator==(Addr b){
        if(ip==b.ip && port==b.port)
            return true;
        else
            return false;
    }
    bool operator!=(Addr b){
        if(*this==b)
            return false;
        else
            return true;
    }
    string ip;
    uint16_t  port;
};
struct  TrCompAddr{
    TrCompAddr(string uHostIP="0.0.0.0:0:0",
              string uReflexIP="0.0.0.0:0"):
            hostAddr(uHostIP),
            reflexAddr(uReflexIP){

    }
    string hostAddr;
    string reflexAddr;
    int len=0;
    int step=0;

};
class connSession {
public:
    muduo::net::TcpConnectionPtr conn;
    vector<TrCompAddr> trCompAddrVect;
    int natType=-1;
//    std::string hostAddr;
    std::string netMask;
//    std::string reflexAddr;
    pthread_t ptd;
    EMTraverseState emTraverseState=EMTState_INIT;
    bool gottenPort=false;
    bool canStart=false;
    friend  std::ostream& operator<<(std::ostream&out,connSession& cs){
        std::cout<<cs.netMask<<"|"<<cs.natType<<"|"<<cs.emTraverseState<<endl;
//        for(int i=0;i<trCompAddrVect.size();++i){
//
//            printf("hostAddr=%s:%d,reflexAddr=%s:%d\n",
//                   trCompAddrVect[i].hostAddr.ip,
//                   trCompAddrVect[i].hostAddr.port,
//                   trCompAddrVect[i].reflexAddr.ip,
//                   trCompAddrVect[i].reflexAddr.port　);
//        }

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
