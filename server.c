#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "init_socket.h"
#include "comm_socket.h"
#include "handler_pars.h"


int 
main(int argc, char **argv){
    server_addr_config config = {
        .host = NULL,
        .port = NULL,
        .daemon_mode = 0,
        .verbose = 0,
        .config_file = NULL,
        .max_connections = 1024
    };
    /* init config */
    init_config(&config);
    /*parse paraments*/
    handler_pars(argc,argv,&config); 
    /*validate paraments*/
    if(validate_args(&config) < 0){
        print_usage(PROGRAM_NAME);
        exit(EXIT_FAILURE);
    }
    /* int socket address */
    struct addrinfo hints;  // getaddrinfo用于过滤结果的提示结构体，可以为 NULL
    memset(&hints, 0, sizeof(hints));
    struct addrinfo *res = NULL;   // getaddrinfo返回的地址链表指针
    init_socket_address(&hints,res,&config);

    /* create socket */
    int socket_fd;
    struct addrinfo *p = NULL;
    int yes = 1;
    init_socket(&socket_fd,res,p,&yes);
    // 释放地址信息链表
    freeaddrinfo(res);
    // 如果没有成功绑定任何地址
    if(p == NULL) {
        fprintf(stderr, "无法绑定到端口 %s\n", config.port);
        return -1;
    }
    



    /*free resource */
    free_config(&config);

}