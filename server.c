#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "parameter.h"
#include "socket.h"
#include "time.h"
#include "server.h"
#include "protocol.h"
#include <sys/types.h>         
#include <sys/socket.h>
//http协议
#include "protocol/http/http.h"
#include "log/log.h"

#include "select/select.h"

// 处理单个客户端的请求（非阻塞）
static int 
handleClientRequest(int client_index, const char *home);

// 检查空闲连接
static void 
checkIdleConnections(void);



//终结服务器
void
deinitServer(PARAMENT *config){
	//释放内存空间
	free(config->host);
    free(config->mode);
}
//初始化服务器
int 
initServer(PARAMENT *config){
	//初始化套接字
	if(initSocket(config->host, config->port, config->mode)==-1){
		return -1;
	}
	return 0;
}

int 
runServer(const char *home) {
    printLog("%d > 资源目录:%s\n", getpid(), home);
    for(;;) {
        int conn;
        // 1. 首先接受客户端连接
        if((conn = acceptClient()) == -1) {
            printLog("accept失败\n");
            continue;
        }
        // 2. 设置初始接收超时
        struct timeval tv = {30, 0};
        setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        // 3. 处理这个连接上的所有请求（Keep-Alive）
        int keep_alive = 1;
        int request_count = 0;
        
        while (keep_alive) {
            HttpRequest request = {};
            request_count++;
            printLog("\n\n%d > ================start===========================\n",getpid()); 
            printLog("%d > 等待第 %d 个请求...\n", getpid(), request_count);
            
            // 4. 接收请求
            char *req = recvRequest(conn);
            
            if(req != NULL && strlen(req) > 0) {
                size_t len = strlen(req);
                ProtocolInfo info = detectProtocol(req, len);
                
                if (info.type == PROTOCOL_WEBSOCKET) {
                    printLog("websocket: %s\n", req);
                } else if (info.type == PROTOCOL_HTTP) {
                    // 处理HTTP请求
                    handleHttpRequest(conn, &request, req, home);
                    // 判断是否保持连接
                    if(shouldCloseConnection(&request) < 0) {
                        keep_alive = 0;
                        printLog("%d > 客户端要求关闭连接\n", getpid());
                    } else {
                        printLog("%d > Keep-Alive连接，等待下一个请求\n", getpid());
                    }
                }
                free(req);
            }else {
                // recv返回NULL或超时
                printLog("%d > 接收超时或连接关闭\n", getpid());
                keep_alive = 0;  // 退出循环
            }
            printLog("%d > ==================end===========================\n",getpid()); 
            // 5. 清理请求结构
            freeHttpRequest(&request);
            
            // 6. 如果保持连接，调整超时时间
            if (keep_alive) {
                // 对于Keep-Alive，设置较短的空闲超时（比如5秒）
                tv.tv_sec = 5;
                tv.tv_usec = 0;
                setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            }
        }
        
        // 7. 关闭连接
        close(conn);
        printLog("%d > 连接关闭，共处理 %d 个请求\n", getpid(), request_count);
    }
    
    return 0;
}


/////////////////////////////////////////////
//初始化select模式的服务器
int
initSelectServer(PARAMENT *config){
    //初始化套接字
    if(initSocket(config->host, config->port, config->mode)==-1){
        return -1;
    }
    return 0;
}
// 处理 select 事件
void 
handleSelectEvents(const char *home) {

    printLog("%d > 检查 master_fds 中的所有描述符:\n", getpid());
    for (int i = 0; i <= server_state.max_fd; i++) {
        if (FD_ISSET(i, &server_state.master_fds)) {
            int flags = fcntl(i, F_GETFD);
            if (flags == -1) {
                printLog("%d >  fd %d 无效! 从 master_fds 中移除\n", getpid(), i);
                FD_CLR(i, &server_state.master_fds);  // 移除坏fd
            } else {
                printLog("%d >  fd %d 有效\n", getpid(), i);
            }
        }
    }

    // 复制 master_fds
    server_state.read_fds = server_state.master_fds;
    //重新设置超时
    struct timeval timeout = {5,0};

    int ready = select(server_state.max_fd + 1,
                      &(server_state.read_fds),
                      NULL, NULL,
                      &timeout);
    
    if (ready < 0) {
        if (errno == EINTR) {
            return;  // 被信号中断，下次再处理
        }
        printLog("%d > select 错误: %s\n", getpid(), strerror(errno));
        return;
    } else if (ready == 0) {
        // 超时，检查空闲连接
        checkIdleConnections();
        return;
    }
    printLog("%d > 开始处理新连接...", getpid());
    
    // 1. 处理新连接
    if(FD_ISSET(server_state.listen_fd, &server_state.read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(server_state.listen_fd,
                              (struct sockaddr*)&client_addr,
                              &addr_len);
        
        if (client_fd >= 0) {
            addClientToSelect(client_fd, &client_addr);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printLog("%d > accept 错误: %s\n", getpid(), strerror(errno));
        }
    }
    
    // 2. 处理客户端数据
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int client_fd = server_state.clients[i].fd;
        if (client_fd > 0 && FD_ISSET(client_fd, &server_state.read_fds)) {
            if (handleClientRequest(i, home) < 0) {
                // 处理失败，移除客户端
                removeClientFromSelect(i);
            }
        }
    }
}
// 运行 select 服务器
int 
runSelectServer(const char *home) {
    printLog("%d > 资源目录: %s (使用 Select 模式)\n", getpid(), home);
    printLog("%d > 服务器启动，等待连接...\n", getpid());
    // 主循环
    while (1) {
        handleSelectEvents(home);
    }
    return 0;
}

// 处理单个客户端的请求（非阻塞）
static int 
handleClientRequest(int client_index, const char *home) {
    ClientInfo *client = &server_state.clients[client_index];
    int conn = client->fd;
    // 1. 设置初始接收超时
    struct timeval tv = {30, 0};
    setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // 2. 处理这个连接上的所有请求（Keep-Alive）
    int keep_alive = 1;
    int request_count = 0;
    
    while (keep_alive) {
        HttpRequest request = {};
        request_count++;
        printLog("\n\n%d > ================start===========================\n",getpid()); 
        printLog("%d > 等待第 %d 个请求...\n", getpid(), request_count);
        
        // 4. 接收请求
        char *req = recvRequest(conn);
        
        if(req != NULL && strlen(req) > 0) {
            size_t len = strlen(req);
            ProtocolInfo info = detectProtocol(req, len);
            
            if (info.type == PROTOCOL_WEBSOCKET) {
                printLog("websocket: %s\n", req);
            } else if (info.type == PROTOCOL_HTTP) {
                // 处理HTTP请求
                handleHttpRequest(conn, &request, req, home);
                // 判断是否保持连接
                if(shouldCloseConnection(&request) < 0) {
                    keep_alive = 0;
                    printLog("%d > 客户端要求关闭连接\n", getpid());
                } else {
                    printLog("%d > Keep-Alive连接，等待下一个请求\n", getpid());
                }
            }
            free(req);
        }else {
            // recv返回NULL或超时
            printLog("%d > 接收超时或连接关闭\n", getpid());
            keep_alive = 0;  // 退出循环
        }
        printLog("%d > ==================end===========================\n",getpid()); 
        // 5. 清理请求结构
        freeHttpRequest(&request);
        
        // 6. 如果保持连接，调整超时时间
        if (keep_alive) {
            // 对于Keep-Alive，设置较短的空闲超时（比如5秒）
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }
    }
    
    // 7. 关闭连接
    close(conn);
    printLog("%d > 连接关闭，共处理 %d 个请求\n", getpid(), request_count);

    return 0;
}
// 检查空闲连接
static void 
checkIdleConnections(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ClientInfo *client = &server_state.clients[i];
        if (client->fd > 0) {
            // 如果连接超过30秒没有活动，断开
            if (now - client->connect_time > 30) {
                printLog("%d > 断开空闲连接: %s:%d\n", getpid(), client->ip, client->port);
                removeClientFromSelect(i);
            }
        }
    }
}


