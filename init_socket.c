#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <fcntl.h>
#include <semaphore.h>
#include <libgen.h> 
#include "http.h"
#include "init_socket.h"
#include "comm_socket.h"
#include "http_curl.h"


// 锁文件描述符
static int lock_fd = -1;
/*
 * 创建并绑定一个监听套接字
 * 返回: 成功返回套接字描述符，失败返回-1
 */
int 
create_listening_socket(const char *host,const char *port)
{
    struct addrinfo hints, *res, *p;
    int sockfd;
    int yes = 1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;         // IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;       // TCP 套接字
    hints.ai_flags    = AI_PASSIVE;        // 用于监听套接字，通配地址
    
    // 获取地址信息链表
    int status = getaddrinfo(host, port, &hints, &res);
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




void printShow(HTTP_REQUEST *hreq){
    printf("%s\n",hreq->method);
    printf("%s\n",hreq->path);
    printf("%s\n",hreq->protocol);
    printf("%s\n",hreq->connection);
}


// 将两个字符指针合并
char *concat_strings_memcpy(const char *str1, const char *str2) {
    if (!str1) str1 = "";
    if (!str2) str2 = "";
    
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    
    char *result = malloc(len1 + len2 + 1);
    if (!result) return NULL;
    
    memcpy(result, str1, len1);
    memcpy(result + len1, str2, len2);
    result[len1 + len2] = '\0';
    
    return result;
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
    char buffer[RECV_BUFF_SIZE] = {};
    char * buf = NULL;
    ssize_t len = 0; //接收到的总字节
    ssize_t n = 0;
    while((n = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        //这里可以实现检测不同协议
        //todo
        buf = realloc(buf,len + n);
        memcpy(buf+len,buffer,n);
        len = len + n;
        //检测接收到的数据中是否以\r\n\r\n结束
        if(strstr(buf,"\r\n\r\n")){ 
            break;
        }
    }
    //解析请求头
    HTTP_REQUEST hreq;
    parseRequest(buf, &hreq);
    //end
    //设置默认文件
    char defaul_path[12] = "/index.html";
    if(strcmp(hreq.path,"/") == 0){
        memcpy(hreq.path,defaul_path,sizeof(defaul_path));
    }
    char *file_name = hreq.path;
    printf("文件名 > %s\n", file_name);

    char local_file[100] = "./www";
    strcat(local_file,file_name);

    int status = 200;
    char *des = "OK";
    //判断文件是否存在
    if(access(local_file, F_OK) != 0) {
        memset(local_file,0,strlen(local_file));
        char not_fund[] = "./www/404.html";
        status = 404;
        des = "Not Found";
        memcpy(local_file,not_fund,sizeof(not_fund));
    }
    //检查读权限
    if (access(local_file, R_OK) != 0) {
        char forbid[] = "./www/403.html";
        status = 403;
        des = "Forbidden";
        memcpy(local_file,forbid,sizeof(forbid));
    }
    printf("本地文件名 > %s\n", local_file);

    //读取文件响应body
    int fd = open(local_file, O_RDONLY);
    char *real_buf = NULL; //响应body
    ssize_t data_len = 0;
    if(fd >= 0){
        char bf[RECV_BUFF_SIZE];
        ssize_t size;
        while((size = read(fd, bf, sizeof(bf) - 1)) > 0){
            real_buf = realloc(real_buf,data_len+size + 1);
            memcpy(real_buf+data_len,bf,size+1);
            data_len = data_len + size;
        }
        if(size < 0){
            free(real_buf);
            perror("read");
        }
        close(fd);
    }
    
    
    //响应header
    char head[500];
    HTTP_RESPOND hres = {
        .status = 200,
        .des = "OK",
        .type= "text/html",
        .length=0,
        .connection = "close"
    };
    memset(hres.protocol,0,sizeof(hres.protocol));
    memcpy(hres.protocol,hreq.protocol,sizeof(hreq.protocol));
    hres.status = status;
    memset(hres.des,0,strlen(hres.des));
    memcpy(hres.des,des,strlen(des)+1);
    hres.length = data_len;
    constructHead(&hres, head);

    //printf("响应头:%s\n",head);
    //fflush(stdout); 

    //发送响应头和响应body
    char *respon = concat_strings_memcpy(head, real_buf);

    send(client_fd, respon, strlen(respon), 0);

    if(n == 0) {
        free(buf);
        printf("客户端 %s:%d 断开连接\n", ipstr, port);
    }else if (n < 0) {
        free(buf);
        perror("recv");
    }else{
        free(buf);
    }
    close(client_fd);
}


// 初始化锁
void 
init_accept_lock() {
    lock_fd = open("/tmp/accept.lock", O_CREAT | O_RDWR, 0644);
    if (lock_fd < 0) {
        perror("open lock file");
        exit(1);
    }
}

// 获取锁
void 
acquire_accept_lock() {
    if(flock(lock_fd, LOCK_EX) < 0) {
        perror("flock");
        exit(1);
    }
}
// 释放锁
void 
release_accept_lock() {
    flock(lock_fd, LOCK_UN);
}



// 工作进程的主循环
void 
worker_loop(int sockfd, int worker_id) {
    while (1) {
        //先获取锁   //防止惊群效应
        acquire_accept_lock();
        
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(sockfd, 
                              (struct sockaddr *)&client_addr, 
                              &addr_len);
        //立即释放锁
        release_accept_lock();
        
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



