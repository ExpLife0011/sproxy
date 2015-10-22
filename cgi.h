#ifndef CGI_H__
#define CGI_H__

#include "peer.h"
#include "guest.h"
#include "parse.h"
#include "binmap.h"

// 可用于CGI_Header的type组件的值
#define CGI_REQUEST       1
#define CGI_RESPONSE      2
#define CGI_DATA          3
#define CGI_VALUE         4

#define CGI_LEN_MAX       (65536 - sizeof(CGI_Header))

struct CGI_Header{
    uint8_t type;
#define CGI_FLAG_ACK      1
    uint8_t flag;
    uint16_t contentLength; //最大65536 - 8
    uint32_t requestId;
}__attribute__((packed));

struct CGI_NVLenPair{
    uint16_t nameLength;
    uint16_t valueLength;
}__attribute__((packed));


#define CGI_NAME_BUFFLEFT  1
struct CGI_NameValue{
    uint32_t name;
    uint32_t value;
}__attribute__((packed));

class Cgi:public Peer{
    char filename[URLLIMIT];
    char cgi_buff[CGI_LEN_MAX];
    size_t cgi_getlen  = 0;
    size_t cgi_outlen  = 0;
    size_t dataleft = 0;
    size_t frameleft = 0;
    uint32_t curid = 1;
    binmap<std::pair<Guest *, uint32_t>, uint32_t> idmap;
    std::set<Peer *> waitlist;
    std::list<CGI_Header *> framequeue;
    virtual void defaultHE(uint32_t events);
    virtual void closeHE(uint32_t events)override;
    virtual void clean(uint32_t errcode, Peer* who, uint32_t id = 0)override;
    enum {WaitHeadr,
          WaitBody,
          HandleRes,
          HandleValue,
          HandleData,
          HandleLeft
    }status = WaitHeadr;
    CGI_Header* SendFrame( const CGI_Header *header, size_t addlen );
    void InProc();
    int  OutProc();
    void Request(HttpReqHeader &req,Guest *guest);
public:
    Cgi(const char *filename);
    virtual ~Cgi();
    virtual ssize_t Write(const void *buff, size_t size, Peer* who, uint32_t id=0)override;
    virtual void wait(Peer *who)override;
    virtual int showerrinfo(int ret,const char *s)override;
    static Cgi *getcgi(HttpReqHeader &req, Guest *guest);
};

#ifdef  __cplusplus
extern "C" {
#endif
int cgi_write(int fd, uint32_t id, const void *buff, size_t len);
#ifdef  __cplusplus
}
#endif
#endif