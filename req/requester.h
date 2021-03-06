#ifndef REQUESTER_H__
#define REQUESTER_H__

#include "base.h"
#include "prot/http_pack.h"

class Responser;

class Requester: public Peer{
protected:
    char sourceip[INET6_ADDRSTRLEN];
    uint16_t  sourceport;
    virtual void defaultHE(uint32_t events) = 0;
public:
    explicit Requester(int fd, struct sockaddr_in6 *myaddr = nullptr);
    explicit Requester(int fd, const char *ip, uint16_t port);
    
    virtual const char *getsrc(const void* index) = 0;
    virtual const char *getip();
    virtual void response(HttpResHeader* res) = 0;
    virtual void transfer(void* index, Responser* res_ptr, void* res_index) = 0;
};

#endif
