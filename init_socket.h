#ifndef __INIT_SOCKET_H
#define __INIT_SOCKET_H
#include "comm_socket.h"
/*init socket*/
void
init_socket(int *sockfd,struct addrinfo *res,struct addrinfo *p, int *yes);
/*init socket address*/
void
init_socket_address(struct addrinfo *hints,
    struct addrinfo *res, 
    server_addr_config * config
);
void print_error(int );
#endif
