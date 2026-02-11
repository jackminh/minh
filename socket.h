#ifndef __SOCKET_H__
#define __SOCKET_H__
#define BACKLOG 10


//初始化socket
int 
initSocket(const char * host, short port, const char *mode);

//等待客户端连接
int
acceptClient(void);

//接收请求数据
char * 
recvRequest(int conn);

//关闭套接字
void
deinitSocket(void);

#endif