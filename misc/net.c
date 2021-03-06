#include "net.h"
#include "common.h"

#include <errno.h>
#include <string.h>
#include <strings.h>
//#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>


const char *DEFAULT_CIPHER_LIST = 
            "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:"
            "kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:"
            "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:"
            "ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:"
            "DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:"
            "!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";

int Bind_any(int fd, short port){
    int flag = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
        LOGOUT("setsockopt SO_REUSEADDR:%s\n", strerror(errno));
        return -1;
    }
#ifdef SO_REUSEPORT
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag)) < 0) {
        LOGOUT("setsockopt SO_REUSEPORT:%s\n", strerror(errno));
        return -1;
    }
#endif
    struct sockaddr_in6 myaddr;
    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin6_family = AF_INET6;
    myaddr.sin6_port = htons(port);
    myaddr.sin6_addr = in6addr_any;

    if (bind(fd, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
        LOGOUT("bind error:%s\n", strerror(errno));
        return -1;
    }

    if ((flag=fcntl(fd, F_GETFL)) == -1) {
        LOGOUT("fcntl get error:%s\n", strerror(errno));
        return -1;
    }
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1){
        LOGOUT("fcntl set error:%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int Listen(short port) {
    int svsk;
    if ((svsk = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        LOGOUT("socket error:%s\n", strerror(errno));
        return -1;
    }


    if(Bind_any(svsk, port))
        return -2;
    //以下设置为keealive配置，非必须，所以不返回错误
    int keepAlive = 1; // 开启keepalive属性
    if(setsockopt(svsk, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) <0)
        LOGE("SO_KEEPALIVE:%s\n", strerror(errno));
    int idle = 60; //一分种没有交互就进行探测
    if(setsockopt(svsk, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))<0)
        LOGE("TCP_KEEPIDLE:%s\n", strerror(errno));
    int intvl = 10; //每10秒探测一次
    if(setsockopt(svsk, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))<0)
        LOGE("TCP_KEEPINTVL:%s\n", strerror(errno));
    int cnt = 3; //探测3次无响应就关闭连接
    if(setsockopt(svsk, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))<0)
        LOGE("TCP_KEEPCNT:%s\n", strerror(errno));

    int enable = 1;
    if(setsockopt(svsk, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable))<0)
        LOGE("TCP_NODELAY:%s\n", strerror(errno));

    int timeout = 60;
    if (setsockopt(svsk, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(timeout))< 0)
        LOGE("TCP_DEFER_ACCEPT:%s\n", strerror(errno));

    if (listen(svsk, 10000) < 0) {
        LOGOUT("listen error:%s\n", strerror(errno));
        return -3;
    }
    return svsk;
}


int Connect(const union sockaddr_un* addr, int type) {
    int fd;
    if ((fd = socket(addr->addr.sa_family, type , 0)) < 0) {
        LOGE("socket error:%s\n", strerror(errno));
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOGE("fcntl error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    if(type == SOCK_STREAM){

        //以下设置为keealive配置，非必须，所以不返回错误
        int keepAlive = 1; // 开启keepalive属性
        if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) <0)
            LOGE("SO_KEEPALIVE:%s\n", strerror(errno));
        int idle = 60; //一分种没有交互就进行探测
        if(setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))<0)
            LOGE("TCP_KEEPIDLE:%s\n", strerror(errno));
        int intvl = 10; //每10秒探测一次
        if(setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))<0)
            LOGE("TCP_KEEPINTVL:%s\n", strerror(errno));
        int cnt = 3; //探测3次无响应就关闭连接
        if(setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt))<0)
            LOGE("TCP_KEEPCNT:%s\n", strerror(errno));

        int enable = 1;
        if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable))<0)
            LOGE("TCP_NODELAY:%s\n", strerror(errno));
    }else{
        if(addr->addr.sa_family == AF_INET && addr->addr_in.sin_addr.s_addr == htonl(INADDR_BROADCAST)){
            int enable=1;
            if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0){
                LOGE("set broadcast:%s\n", strerror(errno));
                close(fd);
                return -1;
            }
        }
    }

    if(protectFd(fd) == 0){
        LOGE("protecd fd error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }
    if (connect(fd, &addr->addr, sizeof(struct sockaddr_in6)) == -1 && errno != EINPROGRESS) {
        LOGE("connecting error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int IcmpSocket(const union sockaddr_un* addr, uint16_t id){
    int fd = -1;
    if(addr->addr.sa_family == AF_INET){
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

        if(fd <= 0){
            LOGE("create icmp socket failed: %s\n", strerror(errno));
            return -1;
        }
        if(id) {
            struct sockaddr_in myaddr;
            bzero(&myaddr, sizeof(myaddr));
            myaddr.sin_family = AF_INET;
            myaddr.sin_port = htons(id);
            myaddr.sin_addr.s_addr = INADDR_ANY;

            if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
                LOGOUT("bind error:%s\n", strerror(errno));
                close(fd);
                return -1;
            }
        }
    }
    if(addr->addr.sa_family == AF_INET6){
        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMPV6);
        if(fd <= 0){
            LOGE("create icmp6 socket failed: %s\n", strerror(errno));
            return -1;
        }

        if(id) {
            struct sockaddr_in6 myaddr;
            bzero(&myaddr, sizeof(myaddr));
            myaddr.sin6_family = AF_INET6;
            myaddr.sin6_port = htons(id);
            myaddr.sin6_addr = in6addr_any;

            if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0) {
                LOGOUT("bind error:%s\n", strerror(errno));
                close(fd);
                return -1;
            }
        }
    }

    if(protectFd(fd) == 0){
        LOGE("protecd fd error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if(connect(fd, &addr->addr, sizeof(union sockaddr_un))){
        LOGE("connect failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    time_t time = 1;
    setsockopt(3, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time));
    setsockopt(3, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
    return fd;
}

const char *getaddrstring(const union sockaddr_un *addr){
    static char buff[100];
    char ip[INET6_ADDRSTRLEN];
    if(addr->addr.sa_family == AF_INET6){
        inet_ntop(AF_INET6, &addr->addr_in6.sin6_addr, ip, sizeof(ip));
        sprintf(buff, "[%s]:%d", ip, ntohs(addr->addr_in6.sin6_port));
    }
    if(addr->addr.sa_family == AF_INET){
        inet_ntop(AF_INET, &addr->addr_in.sin_addr, ip, sizeof(ip));
        sprintf(buff, "%s:%d", ip, ntohs(addr->addr_in.sin_port));
    }
    return buff;
}


#ifndef __ANDROID__
#include <ifaddrs.h>
const char *getlocalip ()
{
    struct ifaddrs *ifap, *ifa;
    static char ips [20][INET6_ADDRSTRLEN];
    memset(ips, 0, sizeof(ips));
    getifaddrs (&ifap);
    int i = 0;
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr == NULL)
            continue;
        if(ifa->ifa_addr->sa_family == AF_INET6){
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ifa->ifa_addr;
            inet_ntop(AF_INET6, &sa->sin6_addr, ips[i], sizeof(ips[0]));
            i++;
        }
        if(ifa->ifa_addr->sa_family == AF_INET){
            struct sockaddr_in *sa = (struct sockaddr_in *) ifa->ifa_addr;
            inet_ntop(AF_INET, &sa->sin_addr, ips[i], sizeof(ips[0]));
            i++;
        }
    }
    freeifaddrs(ifap);
    return (char *)ips;
}
#else
const char *getlocalip (){
    return NULL;
}
#endif

