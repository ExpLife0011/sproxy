#ifndef __NET__H__
#define __NET__H__



#define HTTPSPORT 443
#define HTTPPORT  80
#define CRLF      "\r\n"


#define  connecttip   "HTTP/1.0 200 Connection established" CRLF CRLF

#define DOMAINLIMIT   256
#define URLLIMIT      2048

#ifdef  __cplusplus
extern "C" {
#endif

int spliturl(const char* url, char* host, char* path , uint16_t* port);
//int ConnectTo(const char* host, int port);

int Connect(struct sockaddr*);
    
#ifdef  __cplusplus
}
#endif



#endif