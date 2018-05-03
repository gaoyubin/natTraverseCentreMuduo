#include "CentreServer.h"
#include "UdpTurnServer.hpp"

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>
#include <json/json.h>

#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
using namespace std;
//using namespace muduo;
// using namespace muduo::net;


typedef enum
{
    NatTypeFail=0,
    NatTypeBlocked,

    NatTypeOpen,
    NatTypeCone,
    NatTypeSymInc,
    NatTypeSymOneToOne,
    NatTypeSymRandom,

    NatTypeUnknown
    //StunTypeConeNat,
    //StunTypeRestrictedNat,
    //StunTypePortRestrictedNat,
    //StunTypeSymNat,


} NatType;
typedef enum{
    TraverseMethodCone,
    TraverseMethodOpen,
    TraverseMethodSymInc,
    TraverseMethodSymRandom,
    TraverseMethodTurn,


}TraverseMethod;
string getUniqueID(){
    static long long static_addition=0;
    time_t now;
    time(&now);// 等同于now = time(NULL)
    string id=to_string(now)+to_string(static_addition);
    //cout<<id<<endl;
    return id;
}

int getInterface(char *localIp, char *netMask, char *netInterface){
    #define  MAX_IFS 4
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];
    int SockFD;

    SockFD = socket(AF_INET, SOCK_DGRAM, 0);
    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(SockFD, SIOCGIFCONF, &ifc) < 0) {
        printf("ioctl(SIOCGIFCONF): %m\n");
        return 0;

    }

    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
        string tmp(inet_ntoa( ( (struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
        //cout<<tmp.size()<<endl;
        //cout<<tmp.c_str()<<endl;
        //cout<<sizeof(tmp.c_str())<<endl;
        if(localIp!=NULL)
            snprintf(localIp,tmp.size()+1, "%s", tmp.c_str());
        //localIp=inet_ntoa( ( (struct sockaddr_in *)  &ifr->ifr_addr)->sin_addr);
        if (ifr->ifr_addr.sa_family == AF_INET && strcmp(localIp,"127.0.0.1")!=0 ) {

            strncpy(ifreq.ifr_name, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if(netInterface!=NULL)
                strncpy(netInterface, ifr->ifr_name,sizeof(ifreq.ifr_name));
            if (ioctl (SockFD, SIOCGIFNETMASK, &ifreq) < 0) {
                printf("SIOCGIFHWADDR(%s): %m\n", ifreq.ifr_name);
                return 0;
            }
            tmp=(inet_ntoa( ( (struct sockaddr_in*)&ifreq.ifr_ifru)->sin_addr));
            //cout<<tmp.size()<<endl;
            if(netMask!=NULL)
                snprintf(netMask,tmp.size()+1, "%s",tmp.c_str());
            break;


        }

    }
    return 0;


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
        if(conn->name()!="nologin"){

            auto it=traversePairMap.find(conn->name());
            if(it!=traversePairMap.end())
                traversePairMap.erase(it);

            connMap.erase(conn->name());
        }

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
void sendRespondCmd(const muduo::net::TcpConnectionPtr& conn,string answerCmd,string state,string extraInfo=""){
    Json::Value outVal;
    outVal["cmd"]="respond";
    outVal["answerCmd"]=answerCmd;
    outVal["state"]=state;
    if(extraInfo!="")
        outVal["extraInfo"]=extraInfo;
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
    char localIP[32];
    getInterface(localIP,NULL,NULL);
    outVal["stunSvrIP"]=localIP;
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

                        string id= getUniqueID();
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
            else if(inVal["cmd"]=="list"){
                string userList;
                for(auto it=connMap.begin();it!=connMap.end();++it){
                    userList+=it->second.conn->name()+" ";
                }
                sendRespondCmd(conn,"list","OK",userList);

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


                        char uHostIP[32]={0},peerHostIP[32]={0},uReflexIP[32],peerReflexIP[32];
                        uint16_t  uHostPort,peerHostPort,uReflexPort,peerReflexPort;
                        Json::Value outUVal;
                        Json::Value outPVal;

                        outUVal["cmd"] = "traverse";
                        outPVal["cmd"] = "traverse";

                        sscanf(connMap[uname].hostAddr.c_str(),"%[^:]:%d",uHostIP,&uHostPort);
                        sscanf(connMap[peerName].hostAddr.c_str(),"%[^:]:%d",peerHostIP,&peerHostPort);

                        sscanf(connMap[uname].reflexAddr.c_str(),"%[^:]:%d",uReflexIP,&uReflexPort);
                        sscanf(connMap[peerName].reflexAddr.c_str(),"%[^:]:%d",peerReflexIP,&peerReflexPort);


                        int checkListIndex=0;


                        if( string(uReflexIP)==peerReflexIP) {
                            uint32_t uiNetMask=ntohl(inet_addr(connMap[uname].netMask.c_str()));
                            uint32_t  uiUNameIP=ntohl(inet_addr(uHostIP));
                            uint32_t  uiPeerNameIP=ntohl(inet_addr(peerHostIP));

                            if (connMap[uname].netMask == connMap[peerName].netMask &&
                                (uiNetMask & uiUNameIP) == (uiNetMask & uiPeerNameIP)) {
                                //sendTraverseCmd(connMap[uname].conn,connMap[uname].hostAddr,connMap[peerName].hostAddr);
                                //sendTraverseCmd(connMap[peerName].conn,connMap[peerName].hostAddr,connMap[uname].hostAddr);


                                outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].hostAddr;
                                outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;
                                outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].hostAddr;
                                outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                                checkListIndex++;

                                cout << "host way" << endl;
                            }
                        }

                        if(connMap[uname].natType==NatTypeOpen){ //u is not nat
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].hostAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"] =TraverseMethodOpen;


                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]  = connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"] = TraverseMethodCone;
                            checkListIndex++;

                        }
                        else if(connMap[peerName].natType==NatTypeOpen){
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"] =TraverseMethodCone;


                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]  = connMap[uname].hostAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"] = TraverseMethodOpen;

                            checkListIndex++;
                        }
                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeCone){
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            checkListIndex++;
                        }
//                        else if(connMap[uname].natType==NatTypeSymInc && connMap[peerName].natType==NatTypeCone){
//                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
//                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodSymInc;
//
//
//                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
//                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodOpen;
//
//                            checkListIndex++;
//                        }
//                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeSymInc){
//                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
//                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodOpen;
//
//                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
//                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodSymInc;
//
//                            checkListIndex++;
//                        }
                        else if(connMap[uname].natType==NatTypeSymInc && connMap[peerName].natType==NatTypeCone){
                            Json::Value outSymClientVal;
                            outSymClientVal["cmd"]="getPort";
                            outSymClientVal["peerAddr"]=connMap[peerName].reflexAddr;
                            sendCodec(connMap[uname].conn,outSymClientVal.toStyledString());

                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;


                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            checkListIndex++;
                        }
                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeSymInc){
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            checkListIndex++;
                        }
                        else if(connMap[uname].natType==NatTypeSymInc && connMap[peerName].natType==NatTypeSymInc){

                        }
                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeSymRandom){
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodSymRandom;

                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;
                            checkListIndex++;
                        }
                        else if(connMap[uname].natType==NatTypeSymRandom && connMap[peerName].natType==NatTypeCone){
                            outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = connMap[peerName].reflexAddr;
                            outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodCone;

                            outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= connMap[uname].reflexAddr;
                            outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodSymRandom;
                            checkListIndex++;
                        }
                        string id= getUniqueID();
                        outUVal["checklist"][to_string(checkListIndex)]["peerAddr"] = id;
                        outUVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodTurn;

                        outPVal["checklist"][to_string(checkListIndex)]["peerAddr"]= id;
                        outPVal["checklist"][to_string(checkListIndex)]["method"]=TraverseMethodTurn;
                        checkListIndex++;

                        outUVal["cnt"]=checkListIndex;
                        outPVal["cnt"]=checkListIndex;
                        sendCodec(conn,outUVal.toStyledString());
                        //for test
                        //usleep(200*1000);
                        sendCodec(connMap[peerName].conn,outPVal.toStyledString());
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
                            string id= getUniqueID();
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

