#ifndef __SELECT_H__
#define __SELECT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


// 定义最大客户端数和缓冲区大小
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 4096




// 客户端信息结构体
typedef struct {
    int fd;                    // 套接字描述符
    struct sockaddr_in addr;   // 客户端地址（或 sockaddr_in6）
    char ip[INET6_ADDRSTRLEN]; // IP地址字符串
    int port;                  // 端口号
    time_t connect_time;       // 连接时间
    
} ClientInfo;


// 服务器状态结构体
typedef struct {
    int listen_fd;                    // 监听套接字
    ClientInfo clients[MAX_CLIENTS];  // 客户端数组
    int client_count;                 // 当前客户端数
    int max_fd;                       // 最大文件描述符
    fd_set read_fds;                  // select使用的fd_set
    fd_set master_fds;                // 主fd_set（保持所有fd）
    struct timeval timeout;           // select超时设置
} ServerState;

// 全局服务器状态
extern ServerState server_state;


//添加客户端到 select 监控
int 
addClientToSelect(int client_fd, struct sockaddr_in *addr);

// 从 select 监控移除客户端
void 
removeClientFromSelect(int client_index);

// 清理 select 服务器资源
void 
cleanupSelectServer(void);


#endif