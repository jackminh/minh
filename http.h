#ifndef __HTTP_H__
#define __HTTP_H__
#include <limits.h>
#include <sys/types.h>
/**
GET /cprogramming/c-standard-library-signal-h.html HTTP/2 \r\n
Host: www.runoob.com \r\n
Connection: keep-alive \r\n
User-Agent: curl/7.64.1 \r\n
Accept: gzip, deflate \r\n\r\n
*/ 
//请求头
typedef struct httprequest{
    char method[32];         //请求方法
    char path[PATH_MAX + 1]; //资源路径
    char protocol[32];       //协议版本
    char connection[32];     //连接状态
}HTTP_REQUEST;

int parseRequest(const char *req,HTTP_REQUEST *hreq);

//响应头
typedef struct httprespond{
    char protocol[32];          //协议版本
    int  status;                //状态码
    char des[32];               //状态描述
    char type[64];              //类型
    off_t length;               //响应体字节数
    char connection[32];        //连接状态
}HTTP_RESPOND;
/**
HTTP/2 404 NOT FOUND \r\n
Server: openresty \r\n
Date: Mon, 05 Jan 2026 13:33:24 GMT \r\n
Content-type: text/html; charset=UTF-8 \r\n
Content-length: 68359 \r\n
Connection: keep-alive \r\n\r\n
*/ 
int constructHead(const HTTP_RESPOND *hres, char *head);

#endif
