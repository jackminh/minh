#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

//协议类型枚举
typedef enum{
    PROTOCOL_UNKNOWN = 0, //未知协议
    PROTOCOL_HTTP,
    PROTOCOL_WEBSOCKET,
    PROTOCOL_MQTT,
    PROTOCOL_SSL_TLS,
}ProtocolType;

//协议检测结果
typedef struct{
    ProtocolType type;      //协议类型
    int is_secure;          //是否安全协议(https/wss)
    int version_major;      //主版本号
    int version_minor;      //次版本号
    char *method;           //http方法
    char *path;             //请求路径
    int content_length;     //内容长度
}ProtocolInfo;

//初始化协议信息
void initProtocolInfo(ProtocolInfo *info);
//释放协议信息内存
void freeProtocolInfo(ProtocolInfo *info);
//检测是否是文本协议
int isTextProtocol(const char *data, size_t len);

// 解析HTTP请求行
int parseHttpRequestLine(const char *line, ProtocolInfo *info);

// 解析HTTP头部
void parseHttpHeaders(const char *data, size_t len, ProtocolInfo *info);

//检测MQTT协议
int detectMqttProtocol(const char *data, size_t len);

// 检测SSL/TLS握手
int detectSslTlsProtocol(const char *data, size_t len);


ProtocolInfo detectProtocol(const char *data, size_t len);

// 打印协议信息
void printProtocolInfo(const ProtocolInfo *info);


#endif
