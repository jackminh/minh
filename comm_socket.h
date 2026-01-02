#ifndef __COMM_SOCKET_H
#define __COMM_SOCKET_H

#ifndef EACCES_MSG
    #define EACCES_MSG "Permission to create a socket of the specified type and/or protocol is denied."
#endif

#ifndef EAFNOSUPPORT_MSG
    #define EAFNOSUPPORT_MSG "The specified address family is not supported."
#endif

#ifndef EMFILE_MSG
    #define EMFILE_MSG "The per-process descriptor table is full. or The system file table is full."
#endif

#ifndef ENFILE_MSG
    #define ENFILE_MSG "The system file table is full."
#endif


#ifndef ENOBUFS_MSG
    #define ENOBUFS_MSG "Insufficient buffer space is available.  The socket cannot be created until sufficient resources are freed."
#endif

#ifndef ENOMEM_MSG
    #define ENOMEM_MSG "Insufficient memory was available to fulfill the request."
#endif

#ifndef EPROTONOSUPPORT_MSG
    #define EPROTONOSUPPORT_MSG "The protocol type or the specified protocol is not supported within this domain."
#endif

#ifndef EPROTOTYPE_MSG
    #define EPROTOTYPE_MSG "The socket type is not supported by the protocol."
#endif

/*command line paraments*/
typedef struct{
    char *host;             /* 服务器地址 */
    char *port;             /* 端口号 */
    int daemon_mode;        /* 是否以守护进程运行 */
    int verbose;            /* 详细输出模式 */
    char *config_file;      /* 配置文件路径 */
    int max_connections;    /* 最大连接数 */
} server_addr_config;
 

#define PROGRAM_NAME  "server"






#endif
