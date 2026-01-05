#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include "init_signal.h"



volatile sig_atomic_t stop_server = 0;
volatile sig_atomic_t reload_config = 0;

// SIGCHLD 处理器 - 回收僵尸进程
void sigchld_handler(int sig) {
	printf("收到信号%d\n",sig);
    int saved_errno = errno;
    // ⭐ 非阻塞地回收所有已终止的子进程
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // 子进程已回收
    }
    errno = saved_errno;
}

// SIGTERM/SIGINT 处理器 - 优雅关闭
void sigterm_handler(int sig) {
	printf("收到信号%d\n",sig);
    stop_server = 1;
    printf("收到终止信号，准备关闭服务器...\n");
}

// SIGHUP 处理器 - 重载配置
void sighup_handler(int sig) {
    reload_config = 1;
    printf("收到信号%d\n",sig);
}

// SIGPIPE 处理器 - 忽略 Broken Pipe
void sigpipe_handler(int sig) {
	printf("收到信号%d\n",sig);
    // 当客户端断开连接时，服务器继续写入会导致 SIGPIPE
    // 忽略这个信号，让 write()/send() 返回 EPIPE 错误
}

//设置所有信号处理器
void 
setup_signals(void){
	struct sigaction sa;
	//1. SIGCHLD 回收子进程
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }

    // 2. SIGTERM - 优雅关闭
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        exit(EXIT_FAILURE);
    }

    // 3. SIGINT - Ctrl+C
    sa.sa_handler = sigterm_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    // 4. SIGHUP - 重载配置
    sa.sa_handler = sighup_handler;
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction SIGHUP");
        exit(EXIT_FAILURE);
    }
    
    // 5. SIGPIPE - 忽略 Broken Pipe
    sa.sa_handler = SIG_IGN;  // 直接忽略
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction SIGPIPE");
        exit(EXIT_FAILURE);
    }
    
    // 6. SIGUSR1 - 自定义信号（比如重开日志）
    sa.sa_handler = SIG_IGN;  // 或自定义处理器
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
    
    // 7. SIGUSR2 - 自定义信号
    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
    
    // 8. SIGQUIT - 快速关闭（Ctrl+\）
    sa.sa_handler = sigterm_handler;  // 或更激进的处理
    if(sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction SIGQUIT");
        exit(EXIT_FAILURE);
    }


}



void 
setup_worker_signals(void){

    struct sigaction sa;
    // ⭐ 工作进程应该忽略 SIGCHLD（除非它自己会fork）
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("worker sigaction SIGCHLD");
    }
    // ⭐ 工作进程响应终止信号
    sa.sa_handler = sigterm_handler;
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("worker sigaction SIGTERM");
    }
    if(sigaction(SIGINT, &sa, NULL) == -1) {
        perror("worker sigaction SIGINT");
    }
    // ⭐ 工作进程响应配置重载
    sa.sa_handler = sighup_handler;
    if(sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("worker sigaction SIGHUP");
    }
    // ⭐ 忽略 SIGPIPE（非常重要！）
    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("worker sigaction SIGPIPE");
    }

}