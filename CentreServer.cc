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
    static_addition++;
    cout<<id<<endl;
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
            if(it!=traversePairMap.end()){
                //connMap.erase(traversePairMap[conn->name()]);
                connMap[traversePairMap[conn->name()]].natType=-1;

                traversePairMap.erase(traversePairMap[conn->name()]);
                traversePairMap.erase(it);
            }


            connMap.erase(conn->name());

        }

    }
    for(auto it=connMap.begin();it!=connMap.end();++it){
        cout<<"the userlist :"<<endl;
        cout<<it->first<<endl;
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
    //test
    cout<<outVal.toStyledString()<<endl;
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
                if(connMap.find(uname)!=connMap.end()){
                    sendRespondCmd(conn,"login","FAIL");
                }
                else{
                    connMap[uname].conn=conn;
                    sendRespondCmd(conn,"login","OK");

                }

                //conn->send("hello");
            }
            else if(inVal["cmd"]=="see") {
                string peerName=inVal["peerName"].asString();
                string peerPwd=inVal["peerPwd"].asString();

                string uname=conn->name();

                //if(peerName=="tl" && peerPwd=="123456")
                {
                    Json::Value sendVal;
                    if(connMap.find(peerName)!=connMap.end()){

                        sendRespondCmd(conn,"see","OK");

                        muduo::net::TcpConnectionPtr peerConn=connMap[peerName].conn;
                        traversePairMap[peerName]=uname;
                        traversePairMap[uname]=peerName;

                        string id= getUniqueID();
                        //cout<<conn->name()<<endl;
                        //cout<<peerConn->name()<<endl;
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
                    string uname=conn->name();
                    int  natType=inVal["natType"].asInt();


                    connMap[uname].natType=natType;
                    connMap[uname].netMask=inVal["netMask"].asString();

//                    connMap[uname].reflexAddr=inVal["reflexAddr"].asString();
//                    connMap[uname].hostAddr=inVal["hostAddr"].asString();
//                    connMap[uname].netMask=inVal["netMask"].asString();
                    connMap[uname].trCompAddrVect.clear();
                    for(int i=0;i<inVal["comp"]["cnt"].asInt();++i){
                        TrCompAddr trCompAddr;
                        trCompAddr.reflexAddr=inVal["comp"][to_string(i)]["reflexAddr"].asString();
                        trCompAddr.hostAddr=inVal["comp"][to_string(i)]["hostAddr"].asString();

                        if(!inVal["comp"][to_string(i)]["len"].isNull()){
                            connMap[uname].gottenPort=true;
                            trCompAddr.len=inVal["comp"][to_string(i)]["len"].asInt();
                        }


                        if(!inVal["comp"][to_string(i)]["step"].isNull())
                            trCompAddr.step=inVal["comp"][to_string(i)]["step"].asInt();
                        connMap[uname].trCompAddrVect.push_back(trCompAddr);
                    }


                    string peerName=traversePairMap[uname];
                    auto it= connMap.find(peerName);
                    if( it!=connMap.end() && it->second.natType!=-1){
                        cout<<"detect success"<<endl;
                        //printf("%s:%d,%s:%d\n",uname.c_str(),connMap[uname].natType,peerName.c_str(),connMap[peerName]);
                        //cout<<connMap[uname]<<endl;
                        for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                            printf("hostAddr=%s,reflexAddr=%s\n",
                                   connMap[uname].trCompAddrVect[i].hostAddr.c_str(),
                                   connMap[uname].trCompAddrVect[i].reflexAddr.c_str());
                        }
                        cout<<"------------------"<<endl;
                        for(int i=0;i<connMap[peerName].trCompAddrVect.size();++i){
                            printf("hostAddr=%s,reflexAddr=%s\n",
                                   connMap[peerName].trCompAddrVect[i].hostAddr.c_str(),
                                   connMap[peerName].trCompAddrVect[i].reflexAddr.c_str());
                        }
                        //cout<<connMap[peerName]<<endl;


                        char uHostIP[32]={0},peerHostIP[32]={0},uReflexIP[32],peerReflexIP[32];
                        uint16_t  uHostPort,peerHostPort,uReflexPort,peerReflexPort;
                        Json::Value uOutVal;
                        Json::Value pOutVal;

                        uOutVal["cmd"] = "traverse";
                        pOutVal["cmd"] = "traverse";
                        uOutVal["comp"]["cnt"] = (int)connMap[uname].trCompAddrVect.size();
                        pOutVal["comp"]["cnt"] = (int)connMap[uname].trCompAddrVect.size();





                        sscanf(connMap[uname].trCompAddrVect[0].hostAddr.c_str(),"%[^:]:%d",uHostIP,&uHostPort);
                        sscanf(connMap[peerName].trCompAddrVect[0].hostAddr.c_str(),"%[^:]:%d",peerHostIP,&peerHostPort);

                        sscanf(connMap[uname].trCompAddrVect[0].reflexAddr.c_str(),"%[^:]:%d",uReflexIP,&uReflexPort);
                        sscanf(connMap[peerName].trCompAddrVect[0].reflexAddr.c_str(),"%[^:]:%d",peerReflexIP,&peerReflexPort);


                        int checkListIndex=0;
                        cout<<"checkListIndex="<<checkListIndex<<endl;
                        cout<<"---------------------"<<endl;
                        cout<<connMap[uname].netMask<<endl;
                        cout<<uReflexIP<<endl;
                        cout<<peerReflexIP<<endl;

                        if(strcmp(uReflexIP,peerReflexIP)==0) {
                            uint32_t  uiNetMask=ntohl(inet_addr(connMap[uname].netMask.c_str()));
                            uint32_t  uiUNameIP=ntohl(inet_addr(uHostIP));
                            uint32_t  uiPeerNameIP=ntohl(inet_addr(peerHostIP));

//                            cout<<uiNetMask<<endl;
//                            cout<<uiUNameIP<<endl;
//                            cout<<uiPeerNameIP<<endl;
                            printf("netmask=%x,uIP=%x,pIP=%x\n",uiNetMask,uiUNameIP,uiPeerNameIP);
                            if (connMap[uname].netMask == connMap[peerName].netMask &&
                                (uiNetMask & uiUNameIP) == (uiNetMask & uiPeerNameIP)) {

                                for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                    //for test
//                                    connMap[peerName].trCompAddrVect[i].hostAddr+="1";
//                                    connMap[uname].trCompAddrVect[i].hostAddr+="1";

                                    uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)] = connMap[peerName].trCompAddrVect[i].hostAddr;
                                    pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].hostAddr;
                                }


                                checkListIndex++;

                                cout << "host way" << endl;
                            }
                        }

                        if(connMap[uname].natType==NatTypeOpen || connMap[peerName].natType==NatTypeOpen){ //u is not nat

                            for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[peerName].trCompAddrVect[i].reflexAddr;
                                pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].reflexAddr;
                            }
                            checkListIndex++;
                            cout<<"invert connection"<<endl;

                        }

                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeCone){
                            for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[peerName].trCompAddrVect[i].reflexAddr;
                                pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].reflexAddr;
                            }
                            checkListIndex++;


                        }
                        else if(connMap[uname].natType==NatTypeSymInc && connMap[peerName].natType==NatTypeCone){
                            if(connMap[uname].gottenPort==false){
                                Json::Value getPortVal;
                                getPortVal["cmd"]="getPort";
                                for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                    getPortVal["comp"][to_string(i)]=connMap[peerName].trCompAddrVect[i].reflexAddr;
                                }
                                sendCodec(connMap[uname].conn,getPortVal.toStyledString());
                                goto end;
                            }
                            else{
                                for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                    uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[peerName].trCompAddrVect[i].reflexAddr;
                                    pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].reflexAddr;

                                    pOutVal["comp"][to_string(i)]["checklist"]["len"]=connMap[uname].trCompAddrVect[i].len;
                                    pOutVal["comp"][to_string(i)]["checklist"]["step"]=connMap[uname].trCompAddrVect[i].step;
                                }
                            }
                            checkListIndex++;



                        }
                        else if(connMap[uname].natType==NatTypeCone && connMap[peerName].natType==NatTypeSymInc){
                            if(connMap[peerName].gottenPort==false){
                                Json::Value getPortVal;
                                getPortVal["cmd"]="getPort";
                                getPortVal["comp"]["cnt"]=(int)connMap[uname].trCompAddrVect.size();
                                for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                    getPortVal["comp"][to_string(i)]=connMap[uname].trCompAddrVect[i].reflexAddr;
                                }
                                sendCodec(connMap[peerName].conn,getPortVal.toStyledString());
                                goto end;

                            }
                            else{
                                for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                    uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[peerName].trCompAddrVect[i].reflexAddr;
                                    pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].reflexAddr;

                                    uOutVal["comp"][to_string(i)]["checklist"]["len"]=connMap[peerName].trCompAddrVect[i].len;
                                    uOutVal["comp"][to_string(i)]["checklist"]["step"]=connMap[peerName].trCompAddrVect[i].step;
                                }
                            }
                            checkListIndex++;

                        }
                        else if(connMap[uname].natType==NatTypeSymInc && connMap[peerName].natType==NatTypeSymInc){
                            for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){
                                uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[peerName].trCompAddrVect[i].reflexAddr;
                                pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= connMap[uname].trCompAddrVect[i].reflexAddr;
                            }
                            checkListIndex++;
                        }

                        //checkListIndex++;
                        for(int i=0;i<connMap[uname].trCompAddrVect.size();++i){

                            char localIP[32];
                            getInterface(localIP,NULL,NULL);
                            uOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)] = string(localIP) +":"+ to_string(UDP_TURN_PORT);
                            pOutVal["comp"][to_string(i)]["checklist"][to_string(checkListIndex)]= string(localIP) +":"+ to_string(UDP_TURN_PORT);



                            string id= getUniqueID();
                            uOutVal["comp"][to_string(i)]["checklist"]["id"]=id;
                            pOutVal["comp"][to_string(i)]["checklist"]["id"]=id;


                            uOutVal["comp"][to_string(i)]["checklist"]["cnt"] = checkListIndex+1;
                            pOutVal["comp"][to_string(i)]["checklist"]["cnt"]= checkListIndex+1;

                        }




                        sendCodec(conn,uOutVal.toStyledString());
                        //for test
                        //usleep(200*1000);
                        sendCodec(connMap[peerName].conn,pOutVal.toStyledString());
end:                    ;
                    }


                }
                else if(inVal["answerCmd"]=="getPort"){

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

