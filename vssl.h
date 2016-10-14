#ifndef SSL_ABSTRACT_H_
#define SSL_ABSTRACT_H_

#include <openssl/ssl.h>
#include <assert.h>
#include <errno.h>

class Ssl{
protected:
    SSL *ssl;
    int get_error(int ret){
        if(ret < 0){
            int error = SSL_get_error(ssl, ret);
            switch (error) {
                case SSL_ERROR_WANT_READ:
                case SSL_ERROR_WANT_WRITE:
                    errno = EAGAIN;
                    break;
                case SSL_ERROR_ZERO_RETURN:
                    ret = 0;
                    errno = 0;
                case SSL_ERROR_SYSCALL:
                    break;
            }
        }
        return ret;
    }
public:
    Ssl(SSL *ssl):ssl(ssl){
        assert(ssl);
    }
    virtual ~Ssl(){
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    virtual ssize_t write(const void *buff, size_t size){
        return get_error(SSL_write(ssl, buff, size));

    }
    virtual ssize_t read(void *buff, size_t size){
        return get_error(SSL_read(ssl, buff, size));
    }
    int accept(){
        return get_error(SSL_accept(ssl));
    }
    int connect(){
        return get_error(SSL_connect(ssl));
    }
    void get_alpn(const unsigned char **s, unsigned int * len){
        SSL_get0_alpn_selected(ssl, s, len);
    }
    int set_alpn(const unsigned char *s, unsigned int len){
        return SSL_set_alpn_protos(ssl, s, len);
    }
    virtual bool is_dtls(){
        return false;
    }
};


#endif