#include "TcpTurnServer.h"

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>

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
            idMap[msg].second=conn;
            turnMap[conn]=idMap[msg].first;
            turnMap[idMap[msg].first]=conn;

        }
        else {
            idMap[msg].first=conn;

        }
    }
    else{
        turnMap[conn]->send(msg);
    }
  //conn->send(msg);
}

