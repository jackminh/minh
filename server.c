#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "init_socket.h"
#include "comm_socket.h"
#include "handler_pars.h"
#include "init_signal.h"
#include "http.h"
#include "conf_parse.h"


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
    //设置主进程的名称
    //strncpy(argv[0], "server-master", 13);
    snprintf(argv[0],strlen(argv[0])+1,"server-master");

    // 设置文件锁 防止工作进程在accept处理连接时出现惊群效应
    init_accept_lock();

    /* int socket address */
    int sockfd = create_listening_socket(config.host,config.port);
    
    //创建子进程来处理多个连接
    pid_t worker_pids[WORKER_COUNT];
    //设置信号处理（在 fork 之前）
    setup_signals();

    for (int i = 0; i < WORKER_COUNT; i++) {
        pid_t pid = fork();
        if(pid < 0) {
            perror("fork");
            for(int j=0;j<i;j++){
                kill(worker_pids[j], SIGTERM);
            }
            sleep(1);
            exit(EXIT_FAILURE);
        }
        if(pid == 0) {  // 工作进程
            free_config(&config);
            // ⭐ 工作进程设置自己的信号处理
            setup_worker_signals();

            char worker_name[64];
            snprintf(worker_name, sizeof(worker_name), 
                    "%s-worker-%d", PROGRAM_NAME, i);
            strncpy(argv[0], worker_name, 15);
            //printf("工作进程 %d 启动 (PID: %d)\n", i, getpid());
            worker_loop(sockfd, i);
            // worker_loop 不应该返回，如果返回了就是错误
            exit(EXIT_FAILURE);
        }else{
            //主进程记录子进程PID
            worker_pids[i] = pid;
        }
    }
    //主进程关闭监听套接字
    close(sockfd);
    //主进程循环:监控和管理子进程
    int running_workers = WORKER_COUNT;

    while(!stop_server && running_workers > 0){
        //检查是否需要重载配置
        //todo


        //检查是否有工作进程异常退出
        for(int i=0;i<WORKER_COUNT;i++){
            if(worker_pids[i] > 0){
                if(kill(worker_pids[i],0) == -1 && errno == ESRCH){
                    worker_pids[i] = 0;
                    running_workers--;
                    //重新启动工作进程
                    //todo
                }
            }
        }

        sleep(1);


    }


    //优雅关闭:通知所有工作进程退出
    printf("主进程开始优雅关闭...");
    
    for(int i=0; i< WORKER_COUNT;i++){
        if(worker_pids[i] > 0){
            //通知工作进程退出
            kill(worker_pids[i],SIGTERM);
        }
    }
    //待工作进程退出
    int timeout = 10;
    while(timeout-- > 0 && running_workers > 0){
        sleep(1);
        for(int i=0;i<WORKER_COUNT;i++){
            if(worker_pids[i] > 0 && kill(worker_pids[i],0) == -1){
                worker_pids[i] = 0;
                running_workers--;
            }
        }
    }
    if(running_workers > 0){
        for(int i=0;i<WORKER_COUNT;i++){
            if(worker_pids[i] > 0){
                kill(worker_pids[i],SIGKILL);
            }
        }
        sleep(1);
    }
    free_config(&config);
    printf("服务器正常关闭\n");
    exit(EXIT_SUCCESS);
}
