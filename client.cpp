#include "guest.h"
#include "guest_sni.h"
#include "net.h"

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

int efd;

template<class T>
class Http_server: public Server{
    virtual void defaultHE(uint32_t events){
        if (events & EPOLLIN) {
            int clsk;
            struct sockaddr_in6 myaddr;
            socklen_t temp = sizeof(myaddr);
            if ((clsk = accept(fd, (struct sockaddr*)&myaddr, &temp)) < 0) {
                LOGE("accept error:%s\n", strerror(errno));
                return;
            }

            int flags = fcntl(clsk, F_GETFL, 0);
            if (flags < 0) {
                LOGE("fcntl error:%s\n", strerror(errno));
                close(clsk);
                return;
            }

            fcntl(clsk, F_SETFL, flags | O_NONBLOCK);
            new T(clsk, &myaddr);
        } else {
            LOGE("unknown error\n");
        }
    }
public:
    Http_server(int fd):Server(fd){}
};

void usage(const char * programe){
    printf("Usage: %s [-t] [-p port] [-h] server[:port]\n"
           "       -p: The port to listen, default is 3333.\n"
           "       -t: Run as a transparent proxy, it will disable -p.\n"
           "       -h: Print this.\n"
           , programe);
}

int main(int argc, char** argv) {
    int oc;
    bool istrans  = false;
    while((oc = getopt(argc, argv, "p:dth")) != -1)
    {
        switch(oc){
        case 'p':
            CPORT = atoi(optarg);
            break;
        case 't':
            istrans = true;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        case '?':
            usage(argv[0]);
            return -1;
        }
    }
    if (argc <= optind) {
        usage(argv[0]);
        return -1;
    }
    spliturl(argv[optind], SHOST, nullptr, &SPORT);
    SSL_library_init();    // SSL初库始化
    SSL_load_error_strings();  // 载入所有错误信息

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    efd = epoll_create(10000);
    
    if(istrans){
        CPORT = 80;
        int sni_svsk;
        if ((sni_svsk = Listen(SOCK_STREAM, 443)) < 0) {
            return -1;
        }
        new Http_server<Guest_sni>(sni_svsk);
    }
    int http_svsk;
    if ((http_svsk = Listen(SOCK_STREAM, CPORT)) < 0) {
        return -1;
    }
    new Http_server<Guest>(http_svsk);
    
    LOGOUT("Accepting connections ...\n");
#ifndef DEBUG
    if (daemon(1, 0) < 0) {
        LOGOUT("start daemon error:%s\n", strerror(errno));
    }
#endif
    while (1) {
        int c;
        struct epoll_event events[20];
        if ((c = epoll_wait(efd, events, 20, 5000)) < 0) {
            if (errno != EINTR) {
                LOGE("epoll wait:%s\n", strerror(errno));
                return 4;
            }
            continue;
        }

        for (int i = 0; i < c; ++i) {
            Con* con = (Con*)events[i].data.ptr;
            (con->*con->handleEvent)(events[i].events);
        }
        
        dnstick();
        hosttick();
        proxy2tick();
    }
    return 0;
}


