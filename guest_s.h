#ifndef __GUEST_S_H__
#define __GUEST_S_H__

#include <openssl/ssl.h>

#include "guest.h"
#include "net.h"



class Guest_s:public Guest {
    SSL *ssl;
protected:
    virtual int Write()override;
    virtual void shakehandHE(uint32_t events);
    virtual int Read(void *buff,size_t size)override;
public:
    Guest_s(int fd,SSL *ssl);
    Guest_s(Guest_s* copy);
    virtual void shakedhand();
    virtual int showerrinfo(int ret,const char *s)override;
    virtual ~Guest_s();
};



#endif