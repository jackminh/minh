#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
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

//主体和端口配置
PARAMENT config = {0};

//终结服务器
void
deinitServer(){
	//释放内存空间
	free(config.host);
}
//初始化服务器
int 
initServer(int argc, char **argv){
	printLog("%d > 开始解析命令行参数\n", getpid());
	if(parseCommandParaments(argc, argv, &config) != 0){
		return -1;
	}
	//初始化套接字
	if(initSocket(config.host, config.port)==-1){
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


