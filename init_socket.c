#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "init_socket.h"
#include "comm_socket.h"

void print_error(int eno){
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


void
init_socket(int *sockfd,struct addrinfo *res,struct addrinfo *p, int *yes){
    // 遍历链表，尝试绑定
    for(p = res; p != NULL; p = p->ai_next){
        // 创建套接字
        *sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(*sockfd == -1) {
            print_error(errno);
            continue; // 尝试下一个地址
        }
        // 设置 SO_REUSEADDR 选项，避免 "Address already in use" 错误
        if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, yes, sizeof(int)) == -1) {
            fprintf(stderr, "setsockopt error");
            close(*sockfd);
            continue;
        }
        // 绑定套接字
        if (bind(*sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*sockfd);
            fprintf(stderr, "bind error");
            continue; // 尝试下一个地址
        }
        break; // 成功绑定
    }
}

void
init_socket_address(struct addrinfo *hints,
    struct addrinfo *res, 
    server_addr_config * config
){
    hints->ai_family   = AF_UNSPEC;       // IPv4 或 IPv6
    hints->ai_socktype = SOCK_STREAM;     // TCP 套接字
    hints->ai_flags    = AI_PASSIVE;      // 用于监听套接字，通配地址
    // 获取地址信息链表
    int status = getaddrinfo(NULL, config->port, hints, &res);
    if(status!=0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    }
}


