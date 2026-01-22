#ifndef __PROTOCOL_HTTP_H__
#define __PROTOCOL_HTTP_H__

typedef struct{
    char key[256];
    char value[256];
}Headers;

// HTTP请求结构
typedef struct {
    char method[16];      // GET, POST等
    char path[256];       // 请求路径
    char version[16];     // HTTP/1.0 或 HTTP/1.1
    Headers h[20];        // 头部字段
    char *body;           // 请求体
    size_t body_length;
} HttpRequest;

// HTTP响应结构
typedef struct {
    int status_code;      //状态码
    char *status_message; //响应元语
    char *content_type;   //响应类型
    size_t content_length;
    char *body;
    int keep_alive;       // 是否保持连接
} HttpResponse;

//判断是否要关闭连接
int 
shouldCloseConnection(HttpRequest *req);

//处理http请求
void 
handleHttpRequest(int conn, HttpRequest *req,const char *request, const char *home_dir);
//解析请求
int 
parseHttpRequest(const char *request, HttpRequest *req);

void 
freeHttpRequest(HttpRequest *req);

void 
freeHttpResponse(HttpResponse *res);

#endif