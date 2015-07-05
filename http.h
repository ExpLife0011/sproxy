#ifndef HTTP_H__
#define HTTP_H__

#include "parse.h"

class HttpBase {
protected:
    char http_buff[HEADLENLIMIT];
    size_t http_getlen = 0;
    size_t http_expectlen;
    virtual void HeaderProc() = 0;
    void ChunkLProc();
    void ChunkBProc();
    void FixLenProc();
    void AlwaysProc();
    virtual ssize_t Read(void* buff, size_t len) = 0;
    virtual void ErrProc(int errcode) = 0;
    virtual ssize_t DataProc(const void *buff, size_t size) = 0;
public:
    void (HttpBase::*Http_Proc)() = &HttpBase::HeaderProc;
};


class HttpRes:public HttpBase{
    virtual void HeaderProc()override final;
protected:
    virtual void ReqProc(HttpReqHeader &req) = 0;
};

class HttpReq:public HttpBase{
    virtual void HeaderProc()override final;
protected:
    bool ignore_body = false;
    virtual void ResProc(HttpResHeader &res);
public:
    explicit HttpReq(bool transparent = false);
};

#endif
