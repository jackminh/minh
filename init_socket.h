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
// 初始化锁
void 
init_accept_lock();
// 获取锁
void 
acquire_accept_lock();
// 释放锁
void 
release_accept_lock();
// 将两个字符指针合并
char 
*concat_strings_memcpy(const char *str1, const char *str2);
#endif
