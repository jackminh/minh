#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "protocol.h"
#include "log/log.h"


void 
initProtocolInfo(ProtocolInfo *info){
    info->type = PROTOCOL_UNKNOWN;
    info->is_secure = 0;
    info->version_major= 0;
    info->version_minor = 0;
    info->method = NULL;
    info->path = NULL;
    info->content_length = -1;
}

// 释放协议信息内存
void freeProtocolInfo(ProtocolInfo *info) {
     if(info->method){ 
         free(info->method);
     }
     if(info->path){ 
         free(info->path);
     }
     initProtocolInfo(info);
}

// 检测是否是文本协议（HTTP/WebSocket都是文本协议）
int isTextProtocol(const char *data, size_t len) {
     // 检查前几个字节是否都是可打印字符或空白字符
     int check_len = len < 100 ? len : 100;
     for(int i = 0; i < check_len; i++){
          unsigned char c = data[i]; //获取第一个字符，占用一个字节(-128-127或 0-255),因为要找ASCII 所以转成unsigned char(0-255范围)
          // 二进制协议可能包含空字
          if(c == 0){ //十进制0对应ASCII ’‘
              return 0;
          }
          if(c < 9 || (c > 13 && c < 32 && c != 27)){ //将传入的字符在ASCII表中无法显示的踢除
              return 0;
          }
          // 允许: 制表符(9)、换行(10)、垂直制表(11)、换页(12)、回车(13)
          // 允许: 可打印字符(32-126)、删除(127)
          // 不允许: 其他控制字符
     }
     return 1;
}


// 解析HTTP请求行
int parseHttpRequestLine(const char *line, ProtocolInfo *info) {
    char method[16] = {0};
    char path[1024] = {0};
    char version[16] = {0};
    // 解析格式: "METHOD PATH HTTP/1.1" "GET / HTTP/2.0"
    if(sscanf(line, "%15s %1023s %15s", method, path, version) != 3) {
        return 0;
    }
    // 提取HTTP版本号
    if(strncmp(version, "HTTP/", 5) == 0) {
        //将version指针移动5位,即指向"HTTP/2.0"的2这里
        sscanf(version + 5, "%d.%d", 
               &info->version_major, &info->version_minor);
    }
    // 复制方法
    info->method = strdup(method);
    // 复制路径
    info->path = strdup(path);
    return 1;
}

// 解析HTTP头部
void parseHttpHeaders(const char *data, size_t len, ProtocolInfo *info) {
    const char *ptr = data;       //指向输入字符串的起始位置
    const char *end = data + len; //指向输入字符串的结束位置
    
    while(ptr < end) {
        //查找行结束
        //GET / HTTP/2.0\r\nHost: baidu.com\r\n
        //strstr 从ptr中查找首次出现\r\n的位置即是换秆位置,line_end指向了这个位置
        const char *line_end = strstr(ptr, "\r\n");
        if(!line_end){
            break;
        }
        // 空行表示头部结束
        if(line_end == ptr){ //如果空行的位置和首地址相等
            break;
        }
        // 解析头部字段
        const char *colon = strchr(ptr, ':'); //从ptr指向的字符串的起始位置开始查找':'位置
        if(colon && colon < line_end){
            size_t name_len = colon - ptr;//Host: baidu.com\r\n
            const char *value_start = colon + 1; //+1是因为:后有一个空格
            //跳过值的首部空格
            while(*value_start == ' ' && value_start < line_end){
                value_start++;
            }
            size_t value_len = line_end - value_start;
            //检查WebSocket升级头
            if(strncasecmp(ptr, "Upgrade", name_len) == 0){
                if(strncasecmp(value_start, "websocket", value_len) == 0){
                    info->type = PROTOCOL_WEBSOCKET;
                }
            }else if(strncasecmp(ptr, "Connection", name_len) == 0){
                // 检查是否包含"Upgrade"
                char value[256];
                size_t copy_len = value_len < sizeof(value)-1 ? value_len : sizeof(value)-1;
                strncpy(value, value_start, copy_len);
                value[copy_len] = '\0';
                // 转换为小写方便比较
                for(char *p = value; *p; p++){ 
                    *p = tolower(*p);
                }
                if(strstr(value, "upgrade")){
                    // 已经有Upgrade头，可能不需要再做其他处理
                }
            }   
            // 检查Sec-WebSocket-Key（WebSocket握手特有）
            else if (strncasecmp(ptr, "Sec-WebSocket-Key", name_len) == 0) {
                info->type = PROTOCOL_WEBSOCKET;
            }
            // 检查Sec-WebSocket-Version
            else if (strncasecmp(ptr, "Sec-WebSocket-Version", name_len) == 0) {
                info->type = PROTOCOL_WEBSOCKET;
            }
            // 检查Content-Length
            else if (strncasecmp(ptr, "Content-Length", name_len) == 0) {
                char length_str[32];
                size_t copy_len = value_len < sizeof(length_str)-1 ? value_len : sizeof(length_str)-1;
                strncpy(length_str, value_start, copy_len);
                length_str[copy_len] = '\0';
                info->content_length = atoi(length_str);
            }
        }  
        ptr = line_end + 2;  // 跳过 \r\n ,将line_end指向的位置移动两个位置即下一行的起始位置
    }
}

// 检测MQTT协议
int detectMqttProtocol(const char *data, size_t len) {
    if(len < 2){  //前两个字节
        return 0;
    }
    // MQTT固定头部的第一个字节
    unsigned char first_byte = (unsigned char)data[0];
    // 提取MQTT控制包类型（高4位）
    unsigned char mqtt_type = (first_byte >> 4) & 0x0F;  //任何二进制位& 1111 都是它自己
    unsigned char flag = first_byte & 0x0F;  //低4位必须是0
    if(flag !=0){
        return 0;
    }
    // CONNECT报文类型为1
    if(mqtt_type == 1 && len >= 8) { 
        // 检查协议名 "MQTT"
        if (data[2] == 0x00 && data[3] == 0x04) {  //第3个字节和第4个字节
            if (memcmp(data + 4, "MQTT", 4) == 0) {//将data指针移动4即指向第5个字节，后面连续的4个字节是MQTT
                return 1;
            }
        }
    }
    return 0;
}

// 检测SSL/TLS握手
int detectSslTlsProtocol(const char *data, size_t len) {
    if(len < 5){  //前5个字节
        return 0;
    }
    // TLS记录层：0x16 = Handshake, 0x14 = ChangeCipherSpec
    // 0x17 = Application Data
    unsigned char content_type = (unsigned char)data[0];  //第1个字节是content_type类型
    // TLS版本：0x03 0x01 = TLS 1.0, 0x03 0x02 = TLS 1.1, 0x03 0x03 = TLS 1.2
    unsigned char major_version = (unsigned char)data[1]; //第2个字节是主版本号
    unsigned char minor_version = (unsigned char)data[2]; //第3个字节是次版本号
    (void)major_version;
    (void)minor_version;
    // 检查是否是TLS握手
    if(content_type == 0x16){  // Handshake
        // 握手类型：0x01 = ClientHello
        if(len > 5 && (unsigned char)data[5] == 0x01){   //第6个字节
            return 1;
        }
    }
    return 0;
}

// 主检测函数
ProtocolInfo detectProtocol(const char *data, size_t len){
    ProtocolInfo info;
    initProtocolInfo(&info);
    if(len == 0){
        return info;
    }
    // 1. 首先检测二进制协议
    // 检测MQTT
    if(detectMqttProtocol(data, len)){
        info.type = PROTOCOL_MQTT;
        return info;
    }
    // 检测SSL/TLS
    if(detectSslTlsProtocol(data, len)){
        info.type = PROTOCOL_SSL_TLS;
        info.is_secure = 1;
        return info;
    }
    // 2. 检测文本协议
    if(isTextProtocol(data, len)){
        // 默认认为是HTTP
        info.type = PROTOCOL_HTTP;
        // 查找请求行结束位置
        const char *line_end = strstr(data, "\r\n");
        if (line_end) {
            // 解析HTTP请求行
            size_t line_len = line_end - data;
            char request_line[1024];
            if (line_len < sizeof(request_line)) {
                strncpy(request_line, data, line_len);
                request_line[line_len] = '\0';    
                if(parseHttpRequestLine(request_line, &info)) {// 成功解析HTTP请求行                  
                    // 检查是否是WebSocket握手
                    parseHttpHeaders(data, len, &info);            
                    // 如果已经确定为WebSocket，更新类型
                    if(info.type == PROTOCOL_WEBSOCKET) {// WebSocket也是基于HTTP的，所以保留HTTP信息
                    }
                }
            }
        }
    }
    return info;
}

// 打印协议信息
void printProtocolInfo(const ProtocolInfo *info){
    printLog("=== 协议检测结果 ===\n");
    switch (info->type) {
        case PROTOCOL_UNKNOWN:
            printLog("类型: 未知协议\n");
            break;
        case PROTOCOL_HTTP:
            printLog("类型: HTTP\n");
            break;
        case PROTOCOL_WEBSOCKET:
            printLog("类型: WebSocket\n");
            break;
        case PROTOCOL_MQTT:
            printLog("类型: MQTT\n");
            break;
        case PROTOCOL_SSL_TLS:
            printLog("类型: SSL/TLS握手\n");
            break;
    }
    if (info->is_secure) {
        printLog("安全: 是 (HTTPS/WSS/MQTTS)\n");
    }
    if (info->method) {
        printLog("方法: %s\n", info->method);
    }
    if (info->path) {
        printLog("路径: %s\n", info->path);
    }
    if (info->version_major > 0) {
        printLog("版本: HTTP/%d.%d\n", info->version_major, info->version_minor);
    }
    if (info->content_length >= 0) {
        printLog("内容长度: %d\n", info->content_length);
    }
    printLog("===================\n");
}

