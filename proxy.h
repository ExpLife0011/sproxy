#ifndef PROXY_H__
#define PROXY_H__

#include "host.h"

#include <openssl/ssl.h>

class Proxy : public Host{
    SSL *ssl = nullptr;
    SSL_CTX *ctx = nullptr;
protected:
    virtual ssize_t Read(void *buff, size_t size)override;
    virtual ssize_t Write(const void *buff, size_t size)override;
    virtual int showerrinfo(int ret, const char *)override;
    virtual void waitconnectHE(uint32_t events)override;
    virtual void shakehandHE(uint32_t events);
public:
    Proxy(HttpReqHeader &req, Guest *guest);
    Proxy(Proxy *const copy);
    virtual ~Proxy();
    int32_t bufleft(Peer *)override;
    virtual int showstatus(char *buff, Peer *who)override;
    static Host *getproxy(HttpReqHeader &req, Guest *guest);
};

#endif
