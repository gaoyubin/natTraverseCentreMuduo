#include "CentreServer.h"

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>
#include <json/json.h>

#include <string>
using namespace std;
//using namespace muduo;
// using namespace muduo::net;



CentreServer::CentreServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr)
  : server_(loop, listenAddr, "CentreServer") {
    server_.setConnectionCallback(
            boost::bind(&CentreServer::onConnection, this, _1));
    server_.setMessageCallback(
            boost::bind(&CentreServer::onMessage, this, _1, _2, _3));
    server_.setCloseCallback(boost::bind(&CentreServer::onRemoveConnection,this,_1));

}
void CentreServer::start()
{
  server_.start();
}
void CentreServer::onRemoveConnection(const muduo::net::TcpConnectionPtr& conn){
    cout<<""<<endl;
}
void CentreServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{

  LOG_INFO << "CentreServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");



}

void sendCodec(const muduo::net::TcpConnectionPtr& conn,string outStr){
    muduo::net::Buffer outBuf;
    outBuf.append(outStr);
    int32_t  len=outStr.size();
    int32_t be32=htonl(len);
    outBuf.prepend(&be32,sizeof(be32));
    conn->send(&outBuf);
}
void sendRespondCmd(const muduo::net::TcpConnectionPtr& conn,string answerCmd,string state){
    Json::Value outVal;
    outVal["cmd"]="respond";
    outVal["answerCmd"]=answerCmd;
    outVal["state"]=state;
    sendCodec(conn,outVal.toStyledString());
}


void sendTraverseCmd(const muduo::net::TcpConnectionPtr& conn,string uAddr,string peerAddr){
    Json::Value outVal;
    outVal["cmd"]="traverse";
    outVal["uAddr"]=uAddr;
    outVal["peerAddr"]=peerAddr;
    sendCodec(conn,outVal.toStyledString());

}
void sendDetectCmd(const muduo::net::TcpConnectionPtr& conn){
    Json::Value outVal;
    outVal["cmd"]="detect";
    sendCodec(conn,outVal.toStyledString());
}
void CentreServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
    int32_t len=buf->peekInt32();//already ntohl
    if(buf->readableBytes()<len)
        return;
    buf->retrieveInt32();
    string msg(buf->retrieveAsString(len));
    //LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "<< "data received at " << time.toString();

    cout<<msg<<endl;

    Json::Reader reader;
    Json::Value inVal;
    if(reader.parse(msg,inVal)){
        if(!inVal["cmd"].isNull())
        {
            if(inVal["cmd"]=="login"){
                string uname=inVal["uname"].asString();
                string pwd=inVal["pwd"].asString();
//                std::cout<<inVal["cmd"].asString()<<std::endl;
//                std::cout<<inVal["c/s"].asString()<<std::endl;
//                std::cout<<uname<<std::endl;
//                std::cout<<pwd<<std::endl;


                connMap[uname].conn=conn;

                sendRespondCmd(conn,"login","OK");
                //conn->send("hello");
            }
            else if(inVal["cmd"]=="see") {
                string peerName=inVal["peerName"].asString();
                string peerPwd=inVal["peerPwd"].asString();

                string uname=inVal["uname"].asString();
                //if(peerName=="tl" && peerPwd=="123456")
                {
                    Json::Value sendVal;
                    if(connMap.find(peerName)!=connMap.end()){

                        sendRespondCmd(conn,"see","OK");

                        muduo::net::TcpConnectionPtr peerConn=connMap[peerName].conn;
                        natTraverseMap[peerName]=uname;
                        natTraverseMap[uname]=peerName;


                        sendDetectCmd(conn);
                        sendDetectCmd(peerConn);

                        //conn->send(sendVal.toStyledString());
                        //peerConn->send(sendVal.toStyledString());
                    }
                    else{
                        sendRespondCmd(conn,"see","FAIL");
                    }


                }

            }
            else if(inVal["cmd"]=="respond"){
                if(inVal["answerCmd"]=="detect"){
                    string uname=inVal["uname"].asString();
                    int  natType=inVal["natType"].asInt();
                    connMap[uname].natType=natType;
                    connMap[uname].reflexAddr=inVal["reflexAddr"].asString();
                    connMap[uname].hostAddr=inVal["hostAddr"].asString();
                    connMap[uname].netMask=inVal["netMask"].asString();

                    string peerName=natTraverseMap[uname];
                    auto it= connMap.find(peerName);
                    if( it!=connMap.end() && it->second.natType!=-1){
                        cout<<"detect success"<<endl;
                        //printf("%s:%d,%s:%d\n",uname.c_str(),connMap[uname].natType,peerName.c_str(),connMap[peerName]);
                        cout<<connMap[uname]<<endl;
                        cout<<connMap[peerName]<<endl;

                        if(connMap[uname].netMask==connMap[peerName].netMask){
                            sendTraverseCmd(connMap[uname].conn,connMap[uname].hostAddr,connMap[peerName].hostAddr);
                            sendTraverseCmd(connMap[peerName].conn,connMap[peerName].hostAddr,connMap[uname].hostAddr);
                        }
                    }
                }
                else if(inVal["answerCmd"]=="traverse"){
                    string uname=inVal["uname"].asString();
                    string canTraverseStr=inVal["state"].asString();
                    if(canTraverseStr=="OK"){
                        connMap[uname].canTraverse=true;
                        if(connMap[natTraverseMap[uname]].canTraverse==true){
                            Json::Value outVal;
                            outVal["cmd"]="start";
                            sendCodec(conn,outVal.toStyledString());

                        }
                    }
                    else{
                        connMap[uname].canTraverse=false;
                        cout<<"traverse fail"<<endl;
                    }

                }
            }


            //Json::Value respJson;
            //respJson["cmd"]="response";
            //respJson["res"]
            //conn->send("sd");
        }
    }




}

