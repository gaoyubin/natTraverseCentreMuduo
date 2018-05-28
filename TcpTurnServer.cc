#include "TcpTurnServer.h"

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>
#include <iostream>
#include <map>
// using namespace muduo;
// using namespace muduo::net;

TcpTurnServer::TcpTurnServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr)
  : server_(loop, listenAddr, "TcpTurnServer")
{
  server_.setConnectionCallback(
      boost::bind(&TcpTurnServer::onConnection, this, _1));
  server_.setMessageCallback(
      boost::bind(&TcpTurnServer::onMessage, this, _1, _2, _3));
}

void TcpTurnServer::start()
{
  server_.start();
}

void TcpTurnServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
  LOG_INFO << "TcpTurnServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
    if(conn->connected()){
        conn->setName("nologin");
        if(addrToConnMap.find(conn->peerAddress().toIpPort())!=addrToConnMap.end()){
            conn->setName("haslogin");
        }
        addrToConnMap[conn->peerAddress().toIpPort()]=conn;


        std::cout<<"the client addr is "<<conn->peerAddress().toIpPort()<<std::endl;
    }else{

    }
}




void TcpTurnServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
  muduo::string msg(buf->retrieveAllAsString());
//  LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
//           << "data received at " << time.toString();
    if(conn->name()=="nologin"){
        conn->setName(msg);
        if(idMap.find(msg)!=idMap.end()){
            idMap[msg].second=conn->peerAddress().toIpPort();

            turnMap[idMap[msg].second]=idMap[msg].first;
            turnMap[idMap[msg].first]=idMap[msg].second;
            //std::cout<<"make pair success"<<std::endl;

            printf("make pair succes %s <-----> %s\n",idMap[msg].first.c_str(),idMap[msg].second.c_str());


        }
        else {
            idMap[msg].first=conn->peerAddress().toIpPort();
            std::cout<<msg<<std::endl;

        }
    }
    else{
        if(turnMap.find(conn->peerAddress().toIpPort())!=turnMap.end())
            addrToConnMap[turnMap[conn->peerAddress().toIpPort()]]->send(msg);
        else
            std::cout<<"no peer"<<std::endl;
    }
  //conn->send(msg);
}

