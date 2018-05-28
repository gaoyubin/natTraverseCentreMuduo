#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>

#include <iostream>
#include <map>
#include <json/json.h>

class UDPSession{
public:
//    UDPSession():heartBeatCnt(0),hasAnswer(false){
//
//    }
    int heartBeatCnt=0;
    bool hasAnswer=false;
};
std::map<std::string, std::pair<struct sockaddr_in, struct sockaddr_in> > idMap;
std::map<struct sockaddr_in, struct sockaddr_in> turnMap;

std::map<struct sockaddr_in,UDPSession>sockSessionMap;
int listenfd;
//std::map<std::string, std::string> sMap;

#include "CentreServer.h"


#define MAX_BUF 10240
#define MAX_EPOLL_SIZE 100

//bool operator==(struct sockaddr_in a,struct sockaddr_in b){
//    if(a.sin_addr.s_addr==b.sin_addr.s_addr && a.sin_port==b.sin_port)
//        return true;
//    else
//        return false;
//}

bool operator<(struct sockaddr_in a, struct sockaddr_in b) {
    if (a.sin_addr.s_addr < b.sin_addr.s_addr)
        return true;
    else if (a.sin_addr.s_addr == b.sin_addr.s_addr) {
        if (a.sin_port < b.sin_port)
            return true;
        else
            return false;
    } else
        return false;
}


int handleRead(int sd) {
    char recvbuf[MAX_BUF + 1];
    int ret;
    struct sockaddr_in cliAddrIn;
    socklen_t cliLen = sizeof(cliAddrIn);

    bzero(recvbuf, MAX_BUF + 1);
    ret = recvfrom(sd, recvbuf, MAX_BUF, 0, (struct sockaddr *) &cliAddrIn, &cliLen);


    if (ret > 0) {
        printf("read recv[ %s ] from sockfd=%d %s:%d\n",
               recvbuf,
               sd,
               inet_ntoa(cliAddrIn.sin_addr),ntohs(cliAddrIn.sin_port));
        if(sockSessionMap[cliAddrIn].hasAnswer==false ){
//            struct sockaddr_in sdAddrIn;//服务器绑定的地址
//            socklen_t sdAddrInLen=sizeof(sockaddr_in);
//            getpeername(sd,(struct sockaddr *)&sdAddrIn,&sdAddrInLen);
//            printf("the sockfd %d addr = %s:%d\n",sd,inet_ntoa(sdAddrIn.sin_addr),ntohs(sdAddrIn.sin_port));
            if(strcmp("OK",recvbuf)==0){
                printf("recv OK\n");
                sockSessionMap[cliAddrIn].hasAnswer=true;
            }
            return 1;
        }
        if(turnMap.find(cliAddrIn)!=turnMap.end()){

            printf("send to %s:%d,total %d\n",
                   inet_ntoa(turnMap[cliAddrIn].sin_addr),
                   ntohs(turnMap[cliAddrIn].sin_port),
                   ret
            );


             if(sendto(sd, recvbuf, ret, 0, (struct sockaddr *) &turnMap[cliAddrIn], sizeof(cliAddrIn))<0){
                 perror("sendto:");
             }

        }
        else{
            printf("sockfd=%d,%s:%d no peer\n",sd,inet_ntoa(cliAddrIn.sin_addr),ntohs(cliAddrIn.sin_port));
        }



    }
    else if(ret==0){
//        if(turnMap.find(cliAddrIn)!=turnMap.end()){
//
//        }
        sockSessionMap[cliAddrIn].heartBeatCnt=0;
    }
//    else {
//        perror("recvfrom:");
//    }



}


void* waitAnswer(void*arg){

    string id((char*)arg);
    printf("waitAnswer id=%s\n",id.c_str());
    free(arg);
    if(idMap.find(id)==idMap.end()){
        printf("can not find the %s in idMap\n",id.c_str());
    }
    struct sockaddr_in peer1=idMap[id].first;
    struct sockaddr_in peer2=idMap[id].second;
    while( sockSessionMap[peer1].hasAnswer==false ||  sockSessionMap[peer2].hasAnswer==false ){

        string outStr="turn OK";
        if(sockSessionMap[peer1].hasAnswer==false){
            sendto(listenfd,outStr.c_str(),outStr.size(),0,(sockaddr*)&peer1,sizeof(sockaddr));
            printf("peer1 send turn OK to %s:%d\n",inet_ntoa(peer1.sin_addr),ntohs(peer1.sin_port));
        }

        if(sockSessionMap[peer2].hasAnswer==false){
            sendto(listenfd, outStr.c_str(), outStr.size(), 0, (sockaddr*)&peer2,sizeof(sockaddr));
            printf("peer2 send turn OK to %s:%d\n",inet_ntoa(peer2.sin_addr),ntohs(peer2.sin_port));
        }


        usleep(200*1000);
    }


}

int acceptUdp(int sd, struct sockaddr_in my_addr) {
    int newfd = -1;
    int ret = 0;
    int reuse = 1;
    char buf[200];
    memset(buf,0,sizeof(buf));
    struct sockaddr_in cliAddrIn;
    socklen_t cliLen = sizeof(cliAddrIn);

    //sleep(1);
    ret = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *) &cliAddrIn, &cliLen);
    printf("accept recv [ %s ] from sockfd=%d %s:%d\n",buf,sd,inet_ntoa(cliAddrIn.sin_addr),ntohs(cliAddrIn.sin_port));
    char idArr[60];
    memset(idArr,0,sizeof(idArr));
    if(sscanf(buf,"id:%s",idArr)!=1)
        return -1;
    string inStr=idArr;
//    Json::Reader reader;
//    Json::Value inVal;
//
//    if(reader.parse(string(buf),inVal)==false)
//        return -1;
//    if(inVal["cmd"].isNull()==true)
//        return -1;
//
//    string inStr = inVal["cmd"].asString();
    if (idMap.find(inStr) != idMap.end()) {
//        if(idMap[inStr].first.sin_addr.s_addr==cliAddrIn.sin_addr.s_addr &&
//                idMap[inStr].first.sin_port==cliAddrIn.sin_port)
//            return -1;
        if(memcmp(&(idMap[inStr].first),&cliAddrIn,sizeof(cliAddrIn))==0)
            return -1;
        if(idMap[inStr].second.sin_port!=0)
            return -1;

        string outTmpStr="accept OK";
        sendto(sd, outTmpStr.c_str(), outTmpStr.size(), 0, (struct sockaddr *) &cliAddrIn, sizeof(struct sockaddr));
        printf("send to %s accept OK\n",inet_ntoa(cliAddrIn.sin_addr));
        idMap[inStr].second = cliAddrIn;
        turnMap[cliAddrIn] = idMap[inStr].first;
        turnMap[idMap[inStr].first] = cliAddrIn;

//        sockSessionMap[cliAddrIn]=UDPSession();
//        sockSessionMap[idMap[inStr].first]=UDPSession();


//        Json::Value  outVal;
//        outVal["cmd"]="OK";
//      string outStr=outVal.toStyledString();

//        std::string outStr("OK");
//        for(int i=0;i<4;i++){
//            sendto(sd, outStr.c_str(), outStr.size(), 0, (struct sockaddr *) &cliAddrIn, sizeof(cliAddrIn));
//            //must notice
//            sendto(sd, outStr.c_str(), outStr.size(), 0, (struct sockaddr *) &idMap[inStr].first, sizeof(cliAddrIn));
//        }

        pthread_t tid;
        //to avoid the pthread_create last arg share
        void *chPtr=malloc(inStr.size()+1);//多预留一个字节存放＇＼０＇
        memset(chPtr,0,inStr.size()+1);
        memcpy(chPtr,inStr.c_str(),inStr.size());
        pthread_create(&tid,NULL,waitAnswer,chPtr);

        std::cout << "the another id="<<inStr << std::endl;
        printf("%s:%d  <------>",
               inet_ntoa(cliAddrIn.sin_addr),
               ntohs(cliAddrIn.sin_port),
               printf("%s:%d\n",inet_ntoa(turnMap[cliAddrIn].sin_addr),
                      ntohs(turnMap[cliAddrIn].sin_port)));



    } else {
        if(idMap[inStr].first.sin_port!=0)
            return -1;

        idMap[inStr].first = cliAddrIn;
        //std::cout <<"the id ="<<inStr << std::endl;
        printf("the id = %s\n",inStr.c_str());
        string outStr="accept OK";
        sendto(sd, outStr.c_str(), outStr.size(), 0, (struct sockaddr *) &cliAddrIn, sizeof(struct sockaddr));

        //std::cout<<idMap[inStr].fi<<std::endl;
    }


    if ((newfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("child socket");
        exit(1);
    } else {
        printf("parent:%d  new:%d\n", sd, newfd);
    }

    ret = setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }

    ret = setsockopt(newfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    if (ret) {
        exit(1);
    }
    ret = bind(newfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
    if (ret) {
        perror("chid bind");
        exit(1);
    } else {

    }
    if (connect(newfd, (struct sockaddr *) &cliAddrIn, sizeof(struct sockaddr)) == -1) {
        perror("chid connect");
        exit(1);
    }
// else {
//        perror("");
//    }

    //sleep(1);
    out:
    return newfd;
}

struct STUhandleAcceptArg{
    int sockfd;
    struct sockaddr_in addr;
    int epfd;

};
void* handleAccept(void*arg){

    STUhandleAcceptArg stuHandlAcceptArg=*(STUhandleAcceptArg*)arg;
    int listenfd=stuHandlAcceptArg.sockfd;
    sockaddr_in svrAddrIn=stuHandlAcceptArg.addr;
    int epfd=stuHandlAcceptArg.epfd;
    //printf("listener:%d\n", n);
    int new_sd;
    struct epoll_event child_ev;

    new_sd = acceptUdp(listenfd, svrAddrIn);
    if(new_sd<0)
        return NULL;
    child_ev.events = EPOLLIN;
    child_ev.data.fd = new_sd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_sd, &child_ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd=%dn", new_sd);
        exit(1);
    }
}

void *turnUDP(void *arg) {
    //idMap["1"]=std::pair<struct sockaddr_in,struct sockaddr_in>();
    //sMap["2"]="sfsdf";
    int  epfd, nfds, n, curfds;
    socklen_t len;
    struct sockaddr_in svrAddrIn, their_addr;
    unsigned int port;
    struct epoll_event ev;
    struct epoll_event events[MAX_EPOLL_SIZE];
    int opt = 1;;
    int ret = 0;

    port=*(int*)(arg);

    if ((listenfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    } else {
        printf("socket OK\n");
    }

    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret) {
        exit(1);
    }

    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    if (ret) {
        exit(1);
    }

    bzero(&svrAddrIn, sizeof(svrAddrIn));
    svrAddrIn.sin_family = PF_INET;
    svrAddrIn.sin_port = htons(port);
    svrAddrIn.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr *) &svrAddrIn, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    } else {
        printf("IP bind OK\n");
    }

    epfd = epoll_create(MAX_EPOLL_SIZE);
    if (epfd == -1) {
        perror("");
    }
    ev.events = EPOLLIN;// | EPOLLET;
    ev.data.fd = listenfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd=%dn", listenfd);
        exit(1);
    } else {
        printf("ep add OK\n");
    }

    while (1) {

        nfds = epoll_wait(epfd, events, sizeof(events) / sizeof(epoll_event), -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }

        for (n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                pthread_t ptd;
                STUhandleAcceptArg stuhandleAcceptArg;
                stuhandleAcceptArg.epfd=epfd;
                stuhandleAcceptArg.addr=svrAddrIn;
                stuhandleAcceptArg.sockfd=listenfd;
                //pthread_create(&ptd,NULL,handleAccept,&stuhandleAcceptArg);
                //std::cout<<"accept"<<std::endl;
                handleAccept(&stuhandleAcceptArg);

                //pthread_create(&ptd,NULL,handleAccept,&stuhandleAcceptArg);
            } else {
                handleRead(events[n].data.fd);
            }
        }
    }
    close(listenfd);
    return 0;
}

