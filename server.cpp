#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <list>

#include <openssl/err.h>

#include "common.h"

using std::list;

list <Guest*> guestlist;


int main(int argc, char** argv)
{
    int svsk, clsk;

    SSL_library_init();    //SSL初库始化
    SSL_load_error_strings();  //载入所有错误信息
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_load_verify_locations(ctx,
                                      "/home/choury/keys/sub.class1.server.ca.pem", "/home/choury/keys/sub.class1.server.ca.crt") != 1)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_set_default_verify_paths(ctx) != 1)
        ERR_print_errors_fp(stderr);


    //加载证书和私钥
    if (SSL_CTX_use_certificate_file(ctx, "/home/choury/keys/vps.crt", SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "/home/choury/keys/vps.key", SSL_FILETYPE_PEM) != 1) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    SSL_CTX_set_verify_depth(ctx, 10);

    if ((svsk = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return 2;
    }

    int flag = 1;
    if (setsockopt(svsk, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
        perror("setsockopt");
        return 3;
    }

    struct sockaddr_in6 myaddr;
    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin6_family = AF_INET6;
    myaddr.sin6_port = htons(SPORT);
    myaddr.sin6_addr = in6addr_any;

    if (bind(svsk, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind error");
        return 4;
    }
    if (listen(svsk, 10000) < 0) {
        perror("listen error");
        return 5;
    }

    signal(SIGPIPE, SIG_IGN);
    fprintf(stderr, "Accepting connections ...\n");
    struct epoll_event event;
    struct epoll_event events[20];
    int efd = epoll_create(10000);
    event.data.ptr = NULL;
    event.events = EPOLLIN;
    epoll_ctl(efd, EPOLL_CTL_ADD, svsk, &event);
    while (1) {
        int i, c;
        if ((c = epoll_wait(efd, events, 20, -1)) < 0) {
            if (errno != EINTR) {
                perror("epoll wait");
                return 6;
            }
            continue;
        }
        for (i = 0; i < c; ++i) {
            if (events[i].data.ptr == NULL) {
                if (events[i].events & EPOLLIN) {
                    socklen_t temp = sizeof(myaddr);
                    if ((clsk = accept(svsk, (struct sockaddr*)&myaddr, &temp)) < 0) {
                        perror("accept error");
                        continue;
                    }

                    int flags = fcntl(clsk, F_GETFL, 0);
                    if (flags < 0) {
                        perror("fcntl error");
                        close(clsk);
                        continue;
                    }
                    fcntl(clsk, F_SETFL, flags | O_NONBLOCK);



                    /* 基于ctx 产生一个新的SSL */
                    SSL* ssl = SSL_new(ctx);
                    /* 将连接用户的socket 加入到SSL */
                    SSL_set_fd(ssl, clsk);
                    
                    Guest* guest = new Guest_s(clsk, efd, ssl);

                    guestlist.push_back(guest);
                    event.data.ptr = guest;
                    event.events = EPOLLIN;
                    epoll_ctl(efd, EPOLL_CTL_ADD, clsk, &event);
                    
                    /* 建立SSL 连接*/
                    int ret=SSL_accept(ssl);
                    if (ret != 1) {
                        switch (SSL_get_error(ssl, ret)) {
                        case SSL_ERROR_WANT_READ:
                            event.events = EPOLLIN;
                            epoll_ctl(efd, EPOLL_CTL_MOD, clsk, &event);
                            break;
                        case SSL_ERROR_WANT_WRITE:
                            event.events = EPOLLOUT;
                            epoll_ctl(efd, EPOLL_CTL_MOD, clsk, &event);
                            break;
                        default:
                            guest->clean();
                            ERR_print_errors_fp(stderr);
                            break;
                        }
                        continue;
                    }
                    guest->connected();
                    
                } else {
                    perror("unknown error");
                    return 7;
                }
            } else {
                Peer* peer = (Peer*)events[i].data.ptr;
                peer->handleEvent(events[i].events);
            }
        }
        for (auto i = guestlist.begin(); i != guestlist.end();) {
            if ((*i)->candelete()) {
                delete *i;
                guestlist.erase(i++);
            } else {
                ++i;
            }
        }
    }
    SSL_CTX_free(ctx);
    close(svsk);
    return 0;
}




