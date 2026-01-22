#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>


#include <strings.h>  //strncasecmp
#include "protocol/http/http.h"
#include "protocol/mime.h"
#include "../../socket.h"
#include "../../protocol.h"
#include "log/log.h"

//定义全局响应头
HttpResponse res;

//==========================声明开始==============================================
static void 
sendStaticFile(int conn, HttpRequest *req ,const char *filepath);
static void
sendErrorResponse(int conn,int status_code,const char *status_message);
// static void
// printHttpRequest(HttpRequest *req);


//========================================================================



//判断是否要关闭连接
int 
shouldCloseConnection(HttpRequest *req){
     for(int j=0;j<20;j++){
        if(strncasecmp((req->h)[j].key,"Connection",10) == 0){
            //Connection 部分
            if(strncasecmp((req->h)[j].value,"close",5) == 0){
                return -1;
            }else if(strncasecmp((req->h)[j].value,"keep-alive",10) == 0 ){
                return 0;
            }
            break;
        }
    }
    return -1;
}

// //打印
// static void
// printHttpRequest(HttpRequest *req){
//     (void)req;
//     // printLog("\n%s\n",req->method);
//     // printLog("%s\n",req->path);
//     // printLog("%s\n",req->version);
//     // if(req->body !=NULL){
//     //     printfLog("%s\n",req->body);
//     // }else{
//     //     printLog("body: \n");
//     // }
//     // printLog("%ld\n",req->body_length);
//     // for(int j=0;j<20;j++){
//     //     printLog("%s: %s\n",(req->h)[j].key,(req->h)[j].value);
//     // }
// }


// 发送静态文件
static void 
sendStaticFile(int conn, HttpRequest *req ,const char *filepath){
    FILE *fd = fopen(filepath,"rb");
    if(fd== NULL){
        fprintf(stderr,"fopen error: %s\n",strerror(errno));
        sendErrorResponse(conn,404,"Not Found");
        close(conn);
        return;
    }
    memset(&res, 0, sizeof(res)); //置空响应结构体

    res.status_code = 200;
    res.status_message = "OK";
    //1.获取文件逻辑大小
    //设置文件指针为结尾
    fseek(fd, 0L, SEEK_END);
    long file_size = ftell(fd);
    //重置文件指针为开头
    fseek(fd,0L,SEEK_SET);
    res.content_length = (size_t) file_size;

    //2.获取文件类型
    char *file_type = strrchr(filepath,'.'); //查找.最后出现的位置
    res.content_type = (char *) malloc(1024); //文件的mime类型
    memset(res.content_type,0,1024);
    for(int i=0; i < (int)(sizeof(s_mime) / sizeof(s_mime[0])) ;i++){
        if(strcmp(s_mime[i].suffix,file_type)==0){
            memcpy(res.content_type,s_mime[i].type, strlen(s_mime[i].type));
            res.content_type[strlen(s_mime[i].type)] = '\0';
            break;
        }
    }
    //3.keep_alive
    res.keep_alive  =  shouldCloseConnection(req) == 0 ? 1 : 0;

    //######发送头部
    char headers[512];
    int headers_len = snprintf(headers, sizeof(headers),
                              "HTTP/1.1 %d %s\r\n"
                              "Server: minhServer\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n"
                              "Connection: %s\r\n"
                              "\r\n",
                              res.status_code,
                              res.status_message,
                              res.content_type,
                              res.content_length,
                              res.keep_alive ? "keep-alive" : "close"
                              );
    if(send(conn, headers, headers_len, 0) < 0){
        fprintf(stderr,"send error: %s\n",strerror(errno));
        sendErrorResponse(conn,404,"Not Found");
        close(conn);
        return ;
    }
    //#####发送body
    char buffer[4096] = {0};
    size_t n = 0;
    while( ( n = fread(buffer, 1, sizeof(buffer), fd) ) > 0){
        if(send(conn,buffer,n,0) < 0){
            break;
        }
    }
    fclose(fd);
}

//发送错误响应
static void
sendErrorResponse(int conn,int status_code,const char *status_message){
    char header_buffer[4096] = {'\0'};
    int header_len;

    size_t content_length = strlen(status_message);

    // 1. 生成响应头
    header_len = snprintf(header_buffer, sizeof(header_buffer),
                         "HTTP/1.1 %d %s\r\n"
                         "Server: minhServer\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: %ld\r\n"
                         "Connection: close\r\n"
                         "\r\n",
                         status_code,
                         status_message,
                         content_length);
    // 2. 发送响应头
    if(send(conn, header_buffer, header_len, 0) < 0){
        fprintf(stderr,"发送响应头失败\n");
        return;
    }

    //3. 发送响应体
    if (send(conn, status_message, content_length, 0) < 0) {
        fprintf(stderr,"发送响应体失败\n");
        return ;
    }

}




//=========================================================================

//处理get请求
static void
handleGetRequest(int conn, HttpRequest *req, const char *home_dir){
    //当请求路径为/时
    char *last_pos = strrchr(req->path, '/');
    if(strncasecmp((last_pos + 1),"",1) == 0){
        memcpy(req->path+1,"index.html",10);
    }
    //路径
    char *home = (char *)malloc(strlen(home_dir) + strlen(req->path) + 1);
    memset(home,0,strlen(home_dir) + strlen(req->path) + 1);
    if(home ==NULL){
        close(conn);
        fprintf(stderr,"复制home_dir失败\n");
        return;
    }
    home[strlen(home_dir) + strlen(req->path)] = '\0';
    memcpy(home,home_dir,strlen(home_dir));
    //拼接路径
    strncat(home, req->path, strlen(req->path));
    //判断路径是否可读
    if(access(home,R_OK) < 0){
        fprintf(stderr,"%s: %s\n", home, strerror(errno));
        sendErrorResponse(conn,404,"Not Found");
        close(conn);
        return ;
    }
    //发送静态文件
    sendStaticFile(conn, req, home);
    (void)free(home);
    home=NULL;
}

//处理post请求
static void
handlePostRequest(int conn,  HttpRequest *req){
    (void)conn;
    (void)req;

}

//处理head请求
static void
handleHeadRequest(int conn,  HttpRequest *req, const char *home_dir){
    (void)conn;
    (void)req;
    (void)home_dir;
}




//清理请求结构
void 
freeHttpRequest(HttpRequest *req){
    if(req->body != NULL){
        (void)free(req->body);
    }
}

void 
freeHttpResponse(HttpResponse *res){
    if(res->body !=NULL){
        (void)free(res->body);
    }
}


//解析请求
int 
parseHttpRequest(const char *request, HttpRequest *req){
    //设置新行的起始位置为开头
    char * new_request = (char *) malloc(strlen(request) + 1);
    if(new_request == NULL){
        return -1;
    }
    memset(new_request,0,strlen(request) + 1);
    memcpy(new_request,request,strlen(request)); //复制request
    new_request[strlen(request)] = '\0';

    //1.开始解析头部
    char *header_end_pos = strstr(new_request,"\r\n\r\n"); //头部结束标识
    if(header_end_pos == NULL){
        return -1;
    }
    char *new_line = new_request; //起始位置


    int i=1; //用来区分http是否为请求的第1行
    int k=0; //标记req->h的数组下标
    while(new_line < header_end_pos){
        char *temp = strstr(new_line,"\r\n"); 

        ssize_t line_len = (temp - new_line); //第几行长度
        char *parse_line = (char *)malloc(line_len+1);
        if(parse_line==NULL){
            return -1;
        }
        memset(parse_line,0,line_len+1);  //初始化parse_line

        memcpy(parse_line,new_line,line_len);
        parse_line[strlen(parse_line)]='\0';
        
        if(temp == NULL){    //结束标准识
            break;
        }
        if(i==1){  //第一行
            char method[16]={0};
            char path[256]={0};
            char version[16]={0};
            if(sscanf(parse_line,"%s %s %s",method,path,version)==3){
                memcpy(req->method,method,strlen(method));
                memcpy(req->path,path,strlen(path));
                memcpy(req->version,version,strlen(version));
            }
        }else{   
            char key[256]={0};
            char value[256]={0};
            if(sscanf(parse_line,"%[^:]: %s",key,value)==2){
                memcpy((req->h)[k].key,key,strlen(key));
                memcpy((req->h)[k].value,value,strlen(value));
            }
            k++;
        }
        free(parse_line);
        parse_line = NULL; //置空指针
        i++;
        new_line = temp + 2;  //跨过\r\n两个字符
    }
   

    //2.开始解析body
    char *body = header_end_pos + 4 ; //body开始位置
    if(*body == '\0'){ //没有body
        return 0;
    }
    for(int j=0;j<20;j++){
        if(strncasecmp((req->h)[j].key,"Content-Length",14) == 0){
            //content-length 部分
            char *endptr = NULL;
            long unsigned int body_length = strtol((req->h)[j].value, &endptr, 10);
            errno=0;
            if(errno!=0){
                return -1;
            }
            if(*endptr != '\0'){
                return -1;
            }
            printLog("%ld,%ld\n",body_length,strlen(body));
            if(body_length != strlen(body)){
                return -1;
            }
            req->body_length = body_length;

            //body部分
            req->body = (char *)malloc(strlen(body)+1);
            if(req->body == NULL){
                return -1;
            }
            (req->body)[strlen(body)] = '\0';
            memset(req->body,0,strlen(body)+1);
            memcpy(req->body,body,strlen(body));

            break;
        }
    }

    //释放
    free(new_request);
    return 0;
}


// 发送完整的HTTP响应
void sendHttpResponse(int conn, HttpResponse *resp) {
    char header_buffer[4096] = {'\0'};
    int header_len;
    
    // 1. 生成响应头
    header_len = snprintf(header_buffer, sizeof(header_buffer),
                         "HTTP/1.1 %d %s\r\n"
                         "Server: minhServer\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: %s\r\n"
                         "\r\n",
                         resp->status_code,
                         resp->status_message,
                         resp->content_type,
                         resp->content_length,
                         resp->keep_alive ? "keep-alive" : "close");
    
    // 2. 发送响应头
    if(send(conn, header_buffer, header_len, 0) < 0){
        fprintf(stderr,"发送响应头失败\n");
        return;
    }
    
    //3. 发送响应体（如果有）
    if(resp->body && resp->content_length > 0) {
        if (send(conn, resp->body, resp->content_length, 0) < 0) {
            fprintf(stderr,"发送响应体失败\n");
            return ;
        }
    }
}


//处理HTTP请求的完整流程
void 
handleHttpRequest(int conn, HttpRequest *req,const char *request, const char *home_dir){
    //1. 解析HTTP请求
    if(parseHttpRequest(request, req) < 0) {
        sendErrorResponse(conn, 400, "Bad Request");
        close(conn);
        return;
    }
    //printHttpRequest(&req);
    //2. 目前暂时只支持get,post,head处理
    if(strncasecmp(req->method, "GET", 3) == 0) {
        handleGetRequest(conn, req, home_dir);
    }else if(strncasecmp(req->method, "POST", 4) == 0) {
        handlePostRequest(conn, req);
    }else if(strncasecmp(req->method, "HEAD", 4) == 0) {
        handleHeadRequest(conn, req, home_dir);
    }else{
        sendErrorResponse(conn, 501, "Not Implemented");
    }
    //3. 根据Connection头决定是否关闭连接,检查请求中的Connection头
    if(shouldCloseConnection(req) < 0){
        close(conn);
    }
    //4. 清理响应结构
    freeHttpResponse(&res);

}
