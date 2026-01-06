#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#define __USE_GNU
#include <string.h>
#include <time.h>


#include <http.h>
 

//解析请求头
int parseRequest(const char *req, HTTP_REQUEST *hreq){
    sscanf(req,"%s%s%s",hreq->method,hreq->path,hreq->protocol);
    char *connection = strcasestr(req,"connection");
    if(connection){
        sscanf(connection,"%*s%s",hreq->connection);
    }
    printf("%d.%ld > [%s][%s][%s][%s]\n",getpid(),syscall(SYS_gettid),
         hreq->method, hreq->path, hreq->protocol, hreq->connection   
            );
    if(strcasecmp(hreq->method,"GET")!=0){
        printf("%d,%ld -> 无效的请求方法\n", getpid(),syscall(SYS_gettid));
        return -1; 
    }
    return 0;
}
//构造响应头
/**
HTTP/2 404 NOT FOUND \r\n
Server: openresty \r\n
Date: Mon, 05 Jan 2026 13:33:24 GMT \r\n
Content-type: text/html; charset=UTF-8 \r\n
Content-length: 68359 \r\n
Connection: keep-alive \r\n\r\n
*/ 
int constructHead(const HTTP_RESPOND *hres, char *head){
    char dateTime[32];
    time_t now = time(NULL);
    strftime(dateTime,sizeof(dateTime),"%a %d %b %Y %T",gmtime(&now));
    sprintf(head,"%s %d %s\r\n"
            "Server: openresty\r\n"
            "Date: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: %s\r\n\r\n",
            hres->protocol,
            hres->status,
            hres->des,
            dateTime,
            hres->type,
            hres->length,
            hres->connection
           );
    return 0;
}
