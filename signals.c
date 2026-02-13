#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include "log/log.h"
#include "signals.h"
#include "parameter.h"



pid_t worker_pids[MAX_PROCESS] = {0};
volatile sig_atomic_t server_running = 1;

// SIGCHLD 信号处理函数
void 
sigchldHandler(int sig) {
    (void)sig;  // 避免未使用参数警告
    int saved_errno = errno; // 保存 errno
    while (1) {
        int status;
        pid_t child_pid = waitpid(-1, &status, WNOHANG);
        if(child_pid <= 0) {
            // 没有更多已结束的子进程
            if(child_pid == -1 && errno != ECHILD) {
                printLog("%d > waitpid错误: %s\n", getpid(), strerror(errno));
            }
            break;
        }
        //查找是哪个工作进程
        for(int i = 0; i < MAX_PROCESS; i++) {
            if (worker_pids[i] == child_pid) {
                printLog("%d > 工作进程 %d (PID: %d) ", getpid(), i, child_pid);
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    printLog("正常退出，状态码: %d", exit_code);
                    if (exit_code != 0 && server_running) {
                        printLog(" - 将重启\n");
                        restartWorker(i);
                    } else {
                        printLog(" - 正常关闭\n");
                        worker_pids[i] = 0;
                    }
                } else if (WIFSIGNALED(status)) {
                    int term_sig = WTERMSIG(status);
                    printLog("被信号终止，信号: %d", term_sig);
                    if (server_running && term_sig != SIGTERM && term_sig != SIGINT) {
                        printLog(" - 将重启（意外终止）\n");
                        restartWorker(i);
                    } else {
                        printLog(" - 正常关闭\n");
                        worker_pids[i] = 0;
                    }
                } else if(WIFSTOPPED(status)) {
                    printLog("被暂停，信号: %d\n", WSTOPSIG(status));
                } else if (WIFCONTINUED(status)) {
                    printLog("已继续执行\n");
                }
                break;
            }
        }
    }
    errno = saved_errno;  // 恢复 errno
}

// SIGINT/SIGTERM 信号处理函数
void 
shutdownHandler(int sig) {
    printLog("\n%d > 收到关闭信号 %d，开始优雅关闭...\n", getpid(), sig);
    server_running = 0;
    //发送 SIGTERM 给所有工作进程
    for(int i = 0; i < MAX_PROCESS; i++) {
        if (worker_pids[i] > 0) {
            printLog("%d > 停止工作进程 %d (PID: %d)\n", getpid(), i, worker_pids[i]);
            kill(worker_pids[i], SIGTERM);
        }
    }
}

// 重启工作进程
void restartWorker(int worker_id) {
    if (worker_pids[worker_id] > 0 && server_running) {
        // 等待一段时间再重启，避免频繁重启
        sleep(2);
        
        pid_t pid = fork();
        if (pid == 0) {
            // 子进程
            printLog("%d > 重启工作进程 %d (新PID: %d)\n", getppid(), worker_id, getpid());
            
            extern int (*server_func)(const char *, PARAMENT *config);
            extern const char *server_home;

            if(server_func && server_home){
                server_func(server_home, server_config); //server_config来自全局变量
            }
            exit(0);
        } else if (pid > 0) {
            worker_pids[worker_id] = pid;
        } else {
            printLog("%d > 重启工作进程失败: %s\n", getpid(), strerror(errno));
        }
    }
}

// 设置所有信号处理器
void 
setupSignalHandlers(void) {
    struct sigaction sa;
    
    // 设置 SIGCHLD 处理器
    sa.sa_handler = sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "设置 SIGCHLD 处理器失败: %s\n", strerror(errno));
        exit(1);
    }
    
    // 设置 SIGINT 和 SIGTERM 处理器
    sa.sa_handler = shutdownHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if(sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "设置 SIGINT 处理器失败: %s\n", strerror(errno));
    }
    
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "设置 SIGTERM 处理器失败: %s\n", strerror(errno));
    }
    
    //忽略 SIGPIPE（避免写已关闭的socket导致程序退出）
    signal(SIGPIPE, SIG_IGN);
    printLog("%d > 信号处理器设置完成\n", getpid());

}