#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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
    strncpy(argv[0], PROGRAM_NAME, strlen(argv[0]));//设置进程的名称
    /* int socket address */
    int sockfd = create_listening_socket(config.port);
    
    // 接受新连接
    struct sockaddr_storage client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(sockfd, 
                          (struct sockaddr *)&client_addr, 
                          &addr_len);
    if(client_fd == -1) {
        perror("accept");
    }
    //创建子进程来处理多个连接
    for (int i = 0; i < WORKER_COUNT; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(pid == 0) {  // 工作进程
            char worker_name[64];
            snprintf(worker_name, sizeof(worker_name), 
                    "%s-worker-%d", PROGRAM_NAME, i);
            strncpy(argv[0], worker_name, strlen(argv[0]));
            printf("工作进程 %d 启动 (PID: %d)\n", i, getpid());
            worker_loop(sockfd, i);
            // worker_loop 不应该返回，如果返回了就是错误
            exit(EXIT_FAILURE);
        }
        // 父进程继续 fork 下一个工作进程
    }
    // 父进程（主进程）在这里等待
    printf("主进程 %d 创建了 %d 个工作进程\n", getpid(), WORKER_COUNT);
    // 等待所有子进程
    int status;
    while(1){
        pid_t pid = wait(&status);
        if(pid == -1) {
            if (errno == ECHILD) {
                printf("所有工作进程已结束\n");
                break;
            }
            perror("wait");
        } else {
            printf("工作进程 %d 结束\n", pid);
        }
    }
    close(sockfd);
    free_config(&config);
    exit(EXIT_SUCCESS);
}
