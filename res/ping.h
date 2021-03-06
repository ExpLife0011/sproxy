#ifndef PING_H__
#define PING_H__

#include "responser.h"
#include "misc/net.h"
#include <list>


class Ping: public Responser{
    Requester *req_ptr;
    void*      req_index;
    char hostname[DOMAINLIMIT];
    uint16_t id;
    bool iserror = false;
    std::list<sockaddr_un> addrs;
    virtual void deleteLater(uint32_t errcode) override;
    virtual void defaultHE(uint32_t events);

    static void Dnscallback(Ping* p, const char *hostname, std::list<sockaddr_un> addrs);
public:
    Ping(const char *host, uint16_t id);
    Ping(HttpReqHeader* req);
    virtual void* request(HttpReqHeader* req) override;
    virtual ssize_t Send(void *buff, size_t size, void* index)override;

    virtual int32_t bufleft(void* index)override;
    virtual bool finish(uint32_t flags, void* index)override;
    virtual void dump_stat()override;
};
#endif
