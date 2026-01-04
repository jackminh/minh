#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "init_socket.h"
#include "comm_socket.h"

/*
 * 创建并绑定一个监听套接字
 * 返回: 成功返回套接字描述符，失败返回-1
 */
int 
create_listening_socket(const char *port)
{
    struct addrinfo hints, *res, *p;
    int sockfd;
    int yes = 1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;         // IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;       // TCP 套接字
    hints.ai_flags    = AI_PASSIVE;        // 用于监听套接字，通配地址
    
    // 获取地址信息链表
    int status = getaddrinfo(NULL, port, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }
    print_address_info(res);

    // 遍历链表，尝试绑定
    for (p = res; p != NULL; p = p->ai_next) {
        // 创建套接字
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            perror("socket");
            continue; // 尝试下一个地址
        }
        
        // 设置 SO_REUSEADDR 选项，避免 "Address already in use" 错误
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            close(sockfd);
            continue;
        }
        
        // 绑定套接字
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue; // 尝试下一个地址
        }
        
        break; // 成功绑定
    }
    // 释放地址信息链表
    freeaddrinfo(res);
    
    // 如果没有成功绑定任何地址
    if (p == NULL) {
        fprintf(stderr, "无法绑定到端口 %s\n", port);
        return -1;
    }
    // 开始监听
    if(listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
void 
handle_client(int client_fd, struct sockaddr_storage *client_addr)
{

    char ipstr[INET6_ADDRSTRLEN];
    int port;
    // 获取客户端地址信息
    if (client_addr->ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)client_addr;
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
        port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)client_addr;
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr));
        port = ntohs(s->sin6_port);
    }
    printf("接受来自 %s:%d 的新连接\n", ipstr, port);
    
    // 处理客户端请求
    char buffer[RECV_BUFF_SIZE];
    ssize_t n;
    while ((n = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("收到消息: %s", buffer);
        // 简单回显
        send(client_fd, buffer, n, 0);
    }
    if(n == 0) {
        printf("客户端 %s:%d 断开连接\n", ipstr, port);
    }else if (n < 0) {
        perror("recv");
    } 
    close(client_fd);
}

// 工作进程的主循环
void 
worker_loop(int sockfd, int worker_id) {
    // 工作进程不需要释放 config，因为只读
    while (1) {
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(sockfd, 
                              (struct sockaddr *)&client_addr, 
                              &addr_len);
        if(client_fd == -1) {
            // 如果是被信号中断，继续
            if(errno == EINTR) continue;
            perror("accept");
            // 严重错误，退出工作进程
            break;
        }
        printf("工作进程 %d (PID: %d) 处理客户端 %d\n", 
               worker_id, getpid(), client_fd);
        handle_client(client_fd, &client_addr);
        close(client_fd);
    }
}


void 
print_address_info(struct addrinfo *ai){
    char ipstr[INET6_ADDRSTRLEN];
    char *address[2] = {NULL,NULL};
    int ports[2] = {0};
    int count = 0;
    for (struct addrinfo *p = ai; p != NULL; p = p->ai_next) {
        void *addr;
        int port;
        // 获取 IP 地址和端口
        if (p->ai_family == AF_INET) {  // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            port = ntohs(ipv4->sin_port);
        } else {  // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            port = ntohs(ipv6->sin6_port);
        }
        // 将 IP 地址转换为字符串
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        address[count] = strdup(ipstr);
        ports[count] = port;
        count++;
    }
    // 格式化输出
    if(count == 1) {
        printf("%s:%d 正在监听中...\n", address[0], ports[0]);
    }else if (count == 2) {
        printf("%s:%d and %s:%d 正在监听中...\n", 
               address[0], ports[0], address[1], ports[1]);
    }else if (count > 2) {
        printf("%s:%d and %s:%d (还有 %d 个地址) 正在监听中...\n", 
               address[0], ports[0], address[1], ports[1], count - 2);
    }
    for(int i=0;i<2;i++){
        if(address[i]!=NULL){
            free(address[i]);
        }
    }
    fflush(stdout);
}

void 
print_error(int eno){
    switch(eno){
        case EACCES:
            fprintf(stderr,"socket error#%d:%s\n",eno,EACCES_MSG);
        case EAFNOSUPPORT:
            fprintf(stderr,"socket error#%d:%s\n",eno,EAFNOSUPPORT_MSG); 
        case EMFILE:
            fprintf(stderr,"socket error#%d:%s\n",eno,EMFILE_MSG);
        case ENFILE:
            fprintf(stderr,"socket error#%d:%s\n",eno,ENFILE_MSG);
        case ENOBUFS:
            fprintf(stderr,"socket error#%d:%s\n",eno,ENOBUFS_MSG);
        case ENOMEM:
            fprintf(stderr,"socket error#%d:%s\n",eno,ENOMEM_MSG);
        case EPROTONOSUPPORT:
            fprintf(stderr,"socket error#%d:%s\n",eno,EPROTONOSUPPORT_MSG);
        case EPROTOTYPE:
            fprintf(stderr,"socket error#%d:%s\n",eno,EPROTOTYPE_MSG);
    }
}


