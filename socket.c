#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "socket.h"
#include "log/log.h"

// 检查缓冲区中是否包含完整的 HTTP 头部
static int 
findHttpHeaderEnd(const char *data, size_t len);
// 从头部中提取 Content-Length
static int 
getContentLength(const char *headers, size_t headers_len);

// 检查是否是分块传输
static int 
isChunkedTransfer(const char *headers, size_t headers_len);

// 检查分块传输是否结束
static int 
isChunkedBodyComplete(const char *body, size_t body_len);


static int sockfd = -1;
//初始化socket
int 
initSocket(const char * host, short port){
	printLog("%d > 解析命令行参数 host:%s, port:%hd\n",getpid(),host,port);
	int yes = 1;
	struct addrinfo hints, *p ,*res;

	char ipstr[INET6_ADDRSTRLEN]; //用于显示
	int port_num;				  //用于显示

	memset(&hints,0,sizeof(hints));   //置空
	hints.ai_family   = PF_UNSPEC;    //接收任何协议
	hints.ai_socktype = SOCK_STREAM;  //接收tcl连接
	hints.ai_protocol = IPPROTO_TCP;  //tcp协议

	char *port_ptr = (char *) malloc(6);  //65535
	if(port_ptr == NULL){
		fprintf(stderr,"port to port_ptr error: %s\n",strerror(errno));
		return -1;
	}
    memset(port_ptr,0,6);
    if(snprintf(port_ptr, 6, "%hu", port)<0){
    	fprintf(stderr,"port_ptr error: %s\n",strerror(errno));
    	free(port_ptr);
		return -1;
    }
    if(getaddrinfo(host, port_ptr, &hints, &res) != 0){
    	fprintf(stderr,"getaddrinfo error: %s\n",strerror(errno));
    	free(port_ptr);
    	return -1;
    }
    //res指向一个addrinfo结构体的链表
    for( p=res; p != NULL; p = p->ai_next ){
    	if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
    		fprintf(stderr,"socket error: %s\n",strerror(errno));
    		continue;
    	}
    	//设置socket选项
    	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) !=0){
    		close(sockfd);
    		fprintf(stderr,"setsockopt error: %s\n",strerror(errno));
    		continue;
    	}
    	//绑定
    	if(bind(sockfd, p->ai_addr, p->ai_addrlen)!=0){
    		close(sockfd);
    		fprintf(stderr,"bind error: %s\n",strerror(errno));
    		continue;
    	}
    	//获取绑定成功后的地址和端口
    	if(p->ai_family == AF_INET){
    		struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
    		inet_ntop(AF_INET,&sin->sin_addr,ipstr,sizeof(ipstr));
    		port_num = ntohs(sin->sin_port);
    	}else{
    		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)p->ai_addr;
    		inet_ntop(AF_INET6,&sin6->sin6_addr,ipstr,sizeof(ipstr));
    		port_num = ntohs(sin6->sin6_port);
    	}
    	struct protoent *pt = getprotobynumber(p->ai_protocol);
    	if(pt==NULL){
    		close(sockfd);
    		fprintf(stderr,"getprotobynumber error: %s\n",strerror(errno));
    		continue;
    	}
    	printLog("%d > 监听地址 %s:%d ,协议: %s(%d)\n",getpid(),ipstr,port_num,pt->p_name,pt->p_proto);
    	break;
    }
    (void)freeaddrinfo(res);
    free(port_ptr); //释放分配的内存
    if( p == NULL ){
    	fprintf(stderr, "无法绑定到端口 %d\n", port);
        return -1;
    }
    //////////////////////////////////
    //获取当前文件描述符标志
    // int flags = fcntl(sockfd, F_GETFL, 0);
    // if(flags < 0){
    //     fprintf(stderr, "fcntl F_GETFL error: %s\n", strerror(errno));
    //     close(sockfd);
    //     return -1;
    // }
    // //添加非阻塞标志 防止accept时阻塞在socket()函数处
    // if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
    //     fprintf(stderr, "fcntl F_SETFL O_NONBLOCK error:%s\n", strerror(errno));
    //     close(sockfd);
    //     return -1;
    // }
    ///////////////////////////////

    //监听sockfd前设置为非阻塞
    if(listen(sockfd,BACKLOG) !=0){
    	close(sockfd);
    	fprintf(stderr, "监听失败:%d\n", port);
        return -1;
	}
	return 0;
}
//接收连接
int
acceptClient(void){
	printLog("%d > 等待客户端连接\n",getpid());
	struct sockaddr_storage cli;
    socklen_t len = sizeof(cli);
 
	int conn;
    if((conn=accept(sockfd, (struct sockaddr *)&cli,&len)) == -1){
    	close(sockfd);
    	fprintf(stderr,"接收套接字失败:%s\n",strerror(errno));
    	return -1;
    }
    char client_addr[INET6_ADDRSTRLEN];
    int  client_port;
    if(cli.ss_family == AF_INET){
    	struct sockaddr_in *sin = (struct sockaddr_in *)&cli;
    	inet_ntop(AF_INET,&sin->sin_addr,client_addr,sizeof(client_addr));
    	client_port = ntohs(sin->sin_port);
    }else{
    	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&cli;
    	inet_ntop(AF_INET6,&sin6->sin6_addr,client_addr,sizeof(client_addr));
    	client_port = ntohs(sin6->sin6_port);
    }
	printLog("%d > 接收到 %s:%d 的连接\n", getpid(),client_addr,client_port);
	//设置 连接conn为非阻塞
	// int flags = fcntl(conn, F_GETFL, 0);
    // if(flags < 0){
    //     fprintf(stderr, "fcntl F_GETFL error: %s\n", strerror(errno));
    //     close(conn);
    //     return -1;
    // }
    // //添加非阻塞标志 防止recv/send时阻塞在accept()函数处
    // if(fcntl(conn, F_SETFL, flags | O_NONBLOCK) < 0) {
    //     fprintf(stderr, "fcntl F_SETFL O_NONBLOCK error:%s\n", strerror(errno));
    //     close(conn);
    //     return -1;
    // }
    ///////////////
    
	return conn;
}



//接收请求数据
char *
recvRequest(int conn) {
    char *req = NULL;           // 存储接收到的数据
    size_t total_len = 0;       // 已接收的总长度
    size_t buffer_size = 4096;  // 初始缓冲区大小
    int header_end_pos = -1;    // 头部结束位置
    int content_length = -1;    // Content-Length 值
    int is_chunked = 0;         // 是否是分块传输
    int request_complete = 0;   // 请求是否完整
    
    // 分配初始缓冲区
    req = (char *)malloc(buffer_size);
    if (!req) {
        fprintf(stderr, "malloc error: %s\n", strerror(errno));
        return NULL;
    }
    memset(req,0,buffer_size);
    printLog("%d > 开始接收客户端发送来的数据\n", getpid());
    
    while (!request_complete) {
        // 计算剩余空间
        size_t remaining = buffer_size - total_len;
        if (remaining == 0) {
            // 扩大缓冲区
            buffer_size *= 2;
            char *new_req = (char *)realloc(req, buffer_size);
            if (!new_req) {
                fprintf(stderr, "realloc error: %s\n", strerror(errno));
                free(req);
                return NULL;
            }
            req = new_req;
            remaining = buffer_size - total_len;
        }
        // 接收数据
        ssize_t size = recv(conn, req + total_len, remaining, 0);
        
        if(size > 0){
            total_len += size;
            
            // 1. 首先查找头部结束标记
            if(header_end_pos == -1) {
                header_end_pos = findHttpHeaderEnd(req, total_len); 
                if(header_end_pos != -1) {
                    // 找到头部结束，提取头部信息
                    char *headers = (char *)malloc(header_end_pos + 1);
                    if (headers) {
                        memset(headers,0,header_end_pos+1);
                        memcpy(headers, req, header_end_pos);
                        headers[header_end_pos] = '\0';
                        printLog("%d > 头部信息\n", getpid());
                        printLog("%s\n",headers);
                        // 获取请求方法（用于判断是否需要 body）
                        char method[16] = {0};
                        char *first_space = strchr(headers, ' ');
                        if(first_space){
                            size_t method_len = first_space - headers;
                            if(method_len < sizeof(method) - 1) {
                                strncpy(method, headers, method_len);
                                method[method_len] = '\0';
                            }
                        }
                        
                        // 对于 GET/HEAD/DELETE/OPTIONS，通常没有 body
                        if(strcasecmp(method, "GET") == 0 ||
                            strcasecmp(method, "HEAD") == 0 ||
                            strcasecmp(method, "DELETE") == 0 ||
                            strcasecmp(method, "OPTIONS") == 0) {
                            
                            // 这些方法请求结束
                            request_complete = 1;
                            free(headers);
                            break;
                        }
                        
                        // 解析 Content-Length
                        content_length = getContentLength(headers, header_end_pos);
                        
                        // 检查是否是分块传输
                        is_chunked = isChunkedTransfer(headers, header_end_pos);
                        
                        free(headers);
                    }
                }
            }
            
            // 2. 根据找到的信息判断请求是否完整
            if (header_end_pos != -1) {
                if (content_length > 0) {
                    // 有 Content-Length，检查 body 是否完整
                    int body_len_expected = content_length;
                    int body_len_actual = total_len - header_end_pos;
                    
                    if (body_len_actual >= body_len_expected) {
                        request_complete = 1;
                        break;
                    }
                } else if (is_chunked) {
                    // 分块传输，检查是否结束
                    const char *body_start = req + header_end_pos;
                    size_t body_len = total_len - header_end_pos;
                    
                    if (isChunkedBodyComplete(body_start, body_len)) {
                        request_complete = 1;
                        break;
                    }
                } else {
                    // 没有 Content-Length 也不是 chunked
                    // 对于 POST/PUT，需要检查是否有额外的结束标记
                    // 或者等待连接关闭
                    // 这里简单判断：如果有数据就认为完整
                    if((int)total_len > header_end_pos) {
                        request_complete = 1;
                        break;
                    }
                }
            }
            
            // 3. 检查缓冲区是否过长（防止攻击）
            if (total_len > 10 * 1024 * 1024) {  // 限制为 10MB
                fprintf(stderr, "请求过大，拒绝处理\n");
                free(req);
                return NULL;
            }
            
        } else if (size == 0) {
            // 连接关闭
            printLog("%d > 客户端关闭连接\n", getpid());
            break;
        } else {
            // 接收错误
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞 socket 暂时没有数据
                // usleep(1000);  // 等待 1ms
                // continue;
                return NULL;
            } else {
                fprintf(stderr, "未接收到客户的数据,recv返回码 %zd: %s\n", size , strerror(errno));
                free(req);
                return NULL;
            }
        }
    }
    
    // 确保以 null 结尾（便于调试，但不影响实际数据）
    if (req && total_len > 0) {
        if (total_len < buffer_size) {
            req[total_len] = '\0';
        } else {
            // 需要扩大缓冲区来添加 null 终止符
            char *new_req = (char *)realloc(req, total_len + 1);
            if (new_req) {
                req = new_req;
                req[total_len] = '\0';
            }
        }
    }
    
    return req;
}



// 检查缓冲区中是否包含完整的 HTTP 头部
static int 
findHttpHeaderEnd(const char *data, size_t len){
    // 查找 \r\n\r\n
    for (size_t i = 0; i + 3 < len; i++) {
        if (data[i] == '\r' && data[i+1] == '\n' && 
            data[i+2] == '\r' && data[i+3] == '\n') {
            return i + 4;  // 返回头部结束的位置（包括 \r\n\r\n）
        }
    }
    return -1;  // 没找到
}



// 从头部中提取 Content-Length
static int 
getContentLength(const char *headers, size_t headers_len){
    const char *content_length_str = "Content-Length:";
    const char *cl_ptr = strstr(headers, content_length_str);
    
    if (!cl_ptr || cl_ptr >= headers + headers_len) {
        return -1;  // 没有 Content-Length
    }
    
    // 跳过 "Content-Length:"
    cl_ptr += strlen(content_length_str);
    
    // 跳过空白字符
    while (cl_ptr < headers + headers_len && isspace(*cl_ptr)) {
        cl_ptr++;
    }
    
    // 解析数字
    int length = 0;
    while (cl_ptr < headers + headers_len && isdigit(*cl_ptr)) {
        length = length * 10 + (*cl_ptr - '0');
        cl_ptr++;
    }
    
    return length;
}


// 检查是否是分块传输
static int 
isChunkedTransfer(const char *headers, size_t headers_len){
    const char *te_str = "Transfer-Encoding:";
    const char *te_ptr = strstr(headers, te_str);
    
    if (!te_ptr || te_ptr >= headers + headers_len) {
        return 0;
    }
    
    te_ptr += strlen(te_str);
    
    // 跳过空白字符
    while (te_ptr < headers + headers_len && isspace(*te_ptr)) {
        te_ptr++;
    }
    
    // 检查是否包含 "chunked"
    const char *chunked_str = "chunked";
    size_t chunked_len = strlen(chunked_str);
    
    if (te_ptr + chunked_len > headers + headers_len) {
        return 0;
    }
    
    return strncasecmp(te_ptr, chunked_str, chunked_len) == 0;
}

// 检查分块传输是否结束
static int 
isChunkedBodyComplete(const char *body, size_t body_len){
    // 分块传输以 "0\r\n\r\n" 结束（5字节）
    const char *end_marker = "0\r\n\r\n";
    int end_len = 5;
    
    if ((int)body_len < end_len) {
        return 0;  // 数据不够，不完整
    }
    
    // 从后向前查找结束标记
    for (int i = body_len - end_len; i >= 0; i--) {
        if (memcmp(body + i, end_marker, end_len) == 0) {
            return 1;  // 找到结束标记
        }
    }
    
    return 0;  // 没找到
}





//关闭套接字
void deinitSocket(){
	close(sockfd);
}
