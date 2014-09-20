#ifndef __GUEST_S_H__
#define __GUEST_S_H__

#include <openssl/ssl.h>

#include "guest.h"




class Guest_s:public Guest {
    SSL *ssl;
    virtual int Write()override;
public:
    Guest_s(int fd,int efd,SSL *ssl);
    virtual ~Guest_s();
    virtual void handleEvent(uint32_t events)override;
    virtual int Read(char *buff,size_t size)override;
    virtual void connected()override;
};



#endif