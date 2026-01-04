#ifndef __INIT_SOCKET_H
#define __INIT_SOCKET_H
#include "comm_socket.h"
/*init socket*/
int create_listening_socket(const char *port);
void print_error(int );
void print_address_info(struct addrinfo *ai);
void 
handle_client(int client_fd, struct sockaddr_storage *client_addr);
void 
worker_loop(int sockfd, int worker_id);
#endif
