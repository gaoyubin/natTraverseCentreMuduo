#include "CentreServer.h"
#include "UdpTurnServer.hpp"

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>
#include <json/json.h>

#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
//using namespace muduo;
// using namespace muduo::net;

string genUniqueID(){
    static long long static_addition=0;
    time_t now;
    time(&now);// 等同于now = time(NULL)
    string id=to_string(now)+to_string(static_addition);
    //cout<<id<<endl;
    return id;
}


CentreServer::CentreServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr)
  : server_(loop, listenAddr, "CentreServer") {
    server_.setConnectionCallback(
            boost::bind(&CentreServer::onConnection, this, _1));
    server_.setMessageCallback(
            boost::bind(&CentreServer::onMessage, this, _1, _2, _3));
    //server_.setCloseCallback(boost::bind(&CentreServer::onRemoveConnection,this,_1));

}
void CentreServer::start()
{
  server_.start();
}
/*
void CentreServer::onRemoveConnection(const muduo::net::TcpConnectionPtr& conn){
    if(conn->name()!="nologin")
        connMap.erase(conn->name());
    cout<<"erase"<<endl;
}
*/
void CentreServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{

  LOG_INFO << "CentreServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");

    //add by gaoyubin
    if(conn->connected())
        conn->setName("nologin");
    else{
        if(conn->name()!="nologin")
            connMap.erase(conn->name());
    }

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
void sendDetectCmd(const muduo::net::TcpConnectionPtr& conn,string id){
    Json::Value outVal;
    outVal["cmd"]="detect";
    outVal["id"]=id;
    sendCodec(conn,outVal.toStyledString());
}
void sendUdpTurnCmd(const muduo::net::TcpConnectionPtr &conn, string &id, int port = UDP_TURN_PORT){
    Json::Value outVal;
    outVal["cmd"]="udpTurn";
    outVal["id"]=id;
    outVal["port"]=port;
    sendCodec(conn,outVal.toStyledString());
}
void CentreServer::ControlClientOnMessage(const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp time){
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

                conn->setName(uname);
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
                        traversePairMap[peerName]=uname;
                        traversePairMap[uname]=peerName;

                        string id=genUniqueID();
                        sendDetectCmd(conn,id);
                        sendDetectCmd(peerConn,id);

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

                    string peerName=traversePairMap[uname];
                    auto it= connMap.find(peerName);
                    if( it!=connMap.end() && it->second.natType!=-1){
                        cout<<"detect success"<<endl;
                        //printf("%s:%d,%s:%d\n",uname.c_str(),connMap[uname].natType,peerName.c_str(),connMap[peerName]);
                        cout<<connMap[uname]<<endl;
                        cout<<connMap[peerName]<<endl;

                        if(connMap[uname].netMask==connMap[peerName].netMask ){
                            uint32_t uiNetMask=inet_addr(connMap[uname].netMask.c_str());
                            char uIP[32]={0},peerIP[32]={0};
                            uint16_t  uPort,peerPort;
                            sscanf(connMap[uname].hostAddr.c_str(),"%[^:]:%d",uIP,&uPort);
                            sscanf(connMap[peerName].hostAddr.c_str(),"%[^:]:%d",peerIP,&peerPort);
                            uint32_t  uiUNameIP=inet_addr(uIP);
                            uint32_t  uiPeerNameIP=inet_addr(peerIP);

                            if(uiNetMask&uiUNameIP == uiNetMask&uiUNameIP){
                                sendTraverseCmd(connMap[uname].conn,connMap[uname].hostAddr,connMap[peerName].hostAddr);
                                sendTraverseCmd(connMap[peerName].conn,connMap[peerName].hostAddr,connMap[uname].hostAddr);
                                cout<<"host way"<<endl;
                            }





                        }
                    }
                }
                else if(inVal["answerCmd"]=="traverse"){
                    //string uname=conn->name();
                    string stateStr=inVal["state"].asString();
                    if(stateStr=="OK"){
                        connMap[conn->name()].emTraverseState=EMTState_OK;
                        if(connMap[traversePairMap[conn->name()]].emTraverseState==EMTState_OK){
                            Json::Value outVal;
                            outVal["cmd"]="start";

                            sendCodec(conn,outVal.toStyledString());
                            sendCodec(connMap[traversePairMap[conn->name()]].conn,outVal.toStyledString());
                            connMap[conn->name()].canStart=true;
                            connMap[traversePairMap[conn->name()]].canStart=true;
                        }
                    }
                    else{
                        connMap[conn->name()].emTraverseState=EMTState_FAIL;
                        if(connMap[traversePairMap[conn->name()]].emTraverseState==EMTState_FAIL){
                            string id=genUniqueID();
                            sendUdpTurnCmd(conn, id);
                            //sleep(1);
                            sendUdpTurnCmd(connMap[traversePairMap[conn->name()]].conn, id);
                        }
                        cout<<"traverse fail"<<endl;
                    }

                }
                else if(inVal["answerCmd"]=="udpTurn"){
                    string stateStr=inVal["state"].asString();
                    if(stateStr=="OK"){
                        connMap[conn->name()].emTraverseState=EMTState_OK;
                        if(connMap[traversePairMap[conn->name()]].emTraverseState==EMTState_OK){
                            Json::Value outVal;
                            outVal["cmd"]="start";

                            sendCodec(conn,outVal.toStyledString());
                            sendCodec(connMap[traversePairMap[conn->name()]].conn,outVal.toStyledString());
                            connMap[conn->name()].canStart=true;
                            connMap[traversePairMap[conn->name()]].canStart=true;
                        }
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
void CentreServer::TurnTcpOnMessage(const muduo::net::TcpConnectionPtr& conn,
               muduo::net::Buffer* buf,
               muduo::Timestamp time){
    //cout<<"turn:"<<endl;
    const muduo::net::TcpConnectionPtr& peerConn=connMap[traversePairMap[conn->name()]].conn;
    peerConn->send(buf);
    cout<<"send to "<<connMap[traversePairMap[conn->name()]].conn->name()<<endl;

}
void CentreServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{

    //if(conn->name()!="nologin" && connMap[conn->name()].canStart==true){
    //    cout<<"turn:"<<endl;
    //    TurnTcpOnMessage(conn,buf,time);
    //}else{
        ControlClientOnMessage(conn,buf,time);
    //}

}

