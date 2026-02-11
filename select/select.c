#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "../log/log.h"
#include "select.h"


ServerState server_state = {0};

//添加客户端到 select 监控
int 
addClientToSelect(int client_fd, struct sockaddr_in *addr) {
    if (server_state.client_count >= MAX_CLIENTS) {
        printLog("%d > 客户端数量已达上限(%d)，拒绝连接\n", getpid(),MAX_CLIENTS);
        close(client_fd);
        return -1;
    }
    // 查找空闲位置
    int index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server_state.clients[i].fd < 0) {
            index = i;
            break;
        }
    }
    
    if (index < 0) {
        printLog("%d > 内部错误：找不到空闲客户端位置\n", getpid());
        close(client_fd);
        return -1;
    }
    
    // 设置客户端套接字为非阻塞
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // 填充客户端信息
    ClientInfo *client = &server_state.clients[index];
    client->fd = client_fd;
    client->port = ntohs(addr->sin_port);
    inet_ntop(AF_INET, &addr->sin_addr, client->ip, sizeof(client->ip));
    client->connect_time = time(NULL);
    
    // 初始超时设置
    struct timeval tv = {30, 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 添加到 fd_set
    FD_SET(client_fd, &server_state.master_fds);
    if (client_fd > server_state.max_fd) {
        server_state.max_fd = client_fd;
    }
    
    server_state.client_count++;
    
    printLog("%d > 新客户端连接: %s:%d (fd: %d, 总数: %d)\n", getpid(),
             client->ip, client->port, client_fd, server_state.client_count);
    
    return index;
}

// 从 select 监控移除客户端
void 
removeClientFromSelect(int client_index) {
    if (client_index < 0 || client_index >= MAX_CLIENTS) {
        return;
    }
    
    ClientInfo *client = &server_state.clients[client_index];
    if (client->fd < 0) {
        return;
    }
    
    printLog("%d > 客户端断开: %s:%d (fd: %d)\n", getpid(),
             client->ip, client->port, client->fd);
    
    // 从 fd_set 中移除
    FD_CLR(client->fd, &server_state.master_fds);
    
    // 关闭套接字
    close(client->fd);
    
    // 更新最大 fd
    if (client->fd == server_state.max_fd) {
        server_state.max_fd = server_state.listen_fd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server_state.clients[i].fd > server_state.max_fd) {
                server_state.max_fd = server_state.clients[i].fd;
            }
        }
    }
    
    // 清空客户端信息
    client->fd = -1;
    server_state.client_count--;
}

// 清理 select 服务器资源
void 
cleanupSelectServer(void) {
    printLog("%d > 清理 select 服务器资源...\n", getpid());
    // 关闭所有客户端连接
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server_state.clients[i].fd > 0) {
            close(server_state.clients[i].fd);
        }
    }
    // 关闭监听套接字
    if (server_state.listen_fd > 0) {
        close(server_state.listen_fd);
    }
    printLog("%d > select 服务器资源清理完成\n", getpid());
}