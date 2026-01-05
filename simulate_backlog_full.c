// simulate_backlog_full.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SMALL_BACKLOG 2
#define CLIENT_COUNT 10

int server_fd;
int accept_paused = 1;  // 1=暂停accept, 0=开始accept

void* server_thread(void* arg) {
    int port = *(int*)arg;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    // ⭐ 设置很小的 backlog
    listen(server_fd, SMALL_BACKLOG);
    
    printf("[服务器] 监听端口 %d, backlog=%d\n", port, SMALL_BACKLOG);
    printf("[服务器] ACCEPT队列容量: %d 个连接\n", SMALL_BACKLOG);
    
    // 第一阶段：不 accept，让队列满
    printf("\n=== 第一阶段：不调用 accept() ===\n");
    printf("队列将逐渐变满，新连接会被拒绝\n");
    sleep(5);  // 给客户端时间连接
    
    // 第二阶段：开始 accept
    printf("\n=== 第二阶段：开始调用 accept() ===\n");
    accept_paused = 0;
    
    int accepted = 0;
    while (accepted < CLIENT_COUNT) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(100000);
                continue;
            }
            break;
        }
        
        accepted++;
        printf("[服务器] 接受第 %d 个连接\n", accepted);
        
        char msg[100];
        sprintf(msg, "你是第 %d 个连接\n", accepted);
        send(client_fd, msg, strlen(msg), 0);
        
        close(client_fd);
    }
    
    printf("[服务器] 总共接受 %d 个连接\n", accepted);
    close(server_fd);
    
    return NULL;
}

void* client_thread(void* arg) {
    int id = *(int*)arg;
    char ipstr[INET_ADDRSTRLEN];
    int port = *(int*)((char*)arg + sizeof(int));
    
    // 稍作延迟，让服务器先启动
    usleep(id * 100000);  // 每个客户端间隔 0.1 秒
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };
    
    // 设置连接超时
    struct timeval timeout = {1, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    printf("[客户端 %02d] 尝试连接... ", id);
    fflush(stdout);
    
    int result = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    if (result == 0) {
        printf("成功 ✓\n");
        
        // 尝试读取服务器响应
        char buffer[1024];
        ssize_t n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            printf("[客户端 %02d] 收到: %s", id, buffer);
        }
        
        close(sockfd);
    } else {
        // 分析连接失败原因
        if (errno == ECONNREFUSED) {
            printf("连接被拒绝 ✗ (队列可能已满)\n");
        } else if (errno == ETIMEDOUT) {
            printf("连接超时 ✗\n");
        } else {
            printf("失败 ✗ (错误: %s)\n", strerror(errno));
        }
    }
    
    free(arg);
    return NULL;
}

int main() {
    int port = 8080;
    pthread_t server_tid;
    pthread_t client_tids[CLIENT_COUNT];
    
    // 启动服务器线程
    pthread_create(&server_tid, NULL, server_thread, &port);
    sleep(1);  // 确保服务器先启动
    
    // 启动多个客户端线程
    printf("\n启动 %d 个客户端连接...\n", CLIENT_COUNT);
    printf("注意：服务器 backlog=%d，且暂时不处理连接\n\n", SMALL_BACKLOG);
    
    for (int i = 0; i < CLIENT_COUNT; i++) {
        int* data = malloc(sizeof(int) * 2);
        data[0] = i + 1;  // 客户端ID
        data[1] = port;   // 端口
        
        pthread_create(&client_tids[i], NULL, client_thread, data);
    }
    
    // 等待所有线程完成
    for (int i = 0; i < CLIENT_COUNT; i++) {
        pthread_join(client_tids[i], NULL);
    }
    
    pthread_join(server_tid, NULL);

    return 0;
}