#ifndef PROXY2_H__
#define PROXY2_H__

#include "responser.h"
#include "prot/http2.h"
#include "misc/vssl.h"

struct ReqStatus{
    Requester *req_ptr;
    void*      req_index;
    int32_t remotewinsize; //对端提供的窗口大小，发送时减小，收到对端update时增加
    int32_t localwinsize; //发送给对端的窗口大小，接受时减小，给对端发送update时增加
};

class Proxy2:public Responser, public Http2Requster {
    uint32_t curid = 1;
    std::map<uint32_t, ReqStatus> statusmap;
    SSL_CTX *ctx;
    Ssl *ssl;
#ifdef __ANDROID__
    uint32_t receive_time;
    uint32_t ping_time;
#endif
protected:
    virtual ssize_t Read(void* buff, size_t len)override;
    virtual ssize_t Write(const void* buff, size_t len)override;

    virtual void PushFrame(Http2_header *header)override;
    virtual void DataProc(const Http2_header *header)override;
    virtual void RstProc(uint32_t id, uint32_t errcode)override;
    virtual void WindowUpdateProc(uint32_t id, uint32_t size)override;
    virtual void PingProc(Http2_header *header)override;
    virtual void ErrProc(int errcode)override;
    virtual void ResProc(HttpResHeader* res)override;
    virtual void GoawayProc(Http2_header * header) override;
    virtual void AdjustInitalFrameWindowSize(ssize_t diff)override;
    virtual void defaultHE(uint32_t events);
public:
    using Responser::request;
    explicit Proxy2(int fd, SSL_CTX *ctx, Ssl *ssl);
    virtual ~Proxy2();


    virtual int32_t bufleft(void* index)override;
    virtual ssize_t Send(void *buff, size_t size, void* index)override;
    virtual void writedcb(void* index)override;
    
    virtual void* request(HttpReqHeader* req)override;
    
    virtual void clean(uint32_t errcode, void* index)override;

    virtual void dump_stat()override;
    static void ping_check(Proxy2 *p);
};

extern Proxy2* proxy2; 

#endif
