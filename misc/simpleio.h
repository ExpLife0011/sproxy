#ifndef SIMPLEIO_H__
#define SIMPLEIO_H__
#include "base.h"

class RBuffer {
    char content[BUF_LEN*2];
    uint16_t len = 0;
public:
    size_t left();
    size_t length();
    size_t add(size_t l);
    const char* data();
    size_t consume(const char*, size_t l);
    char* end();
};

class CBuffer {
    char content[BUF_LEN*2];
    uint32_t begin_pos = 0;
    uint32_t end_pos = 0;
public:
    size_t left();
    size_t length();
    void add(size_t l);
    const char* data();
    void consume(const char* data, size_t l);
    char* end();
};

class FdRWer: public RWer{
protected:
    RBuffer rb;
    uint16_t port = 0;
    Protocol protocol;
    char     hostname[DOMAINLIMIT] = {0};
    bool     packet_mode = false;
    std::queue<sockaddr_un> addrs;
    virtual void waitconnectHE(uint32_t events);
    virtual void defaultHE(uint32_t events);
    void connect();
    void reconnect(int error);
    static void Dnscallback(FdRWer* rwer, const char *hostname, std::list<sockaddr_un> addrs);
    static int  con_timeout(FdRWer* rwer);

    virtual ssize_t Read(void* buff, size_t len);
    virtual ssize_t Write(const void* buff, size_t len) override;
public:
    FdRWer(int fd, bool packet_mode, std::function<void(int ret, int code)> errorCB);
    FdRWer(const char* hostname, uint16_t port, Protocol protocol, std::function<void(int ret, int code)> errorCB);
    virtual ~FdRWer();

    //for read buffer
    virtual size_t rlength() override;
    virtual const char *data() override;
    virtual void consume(const char*, size_t l) override;
};

#endif
