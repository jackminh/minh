#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include "signals.h"
#include "process.h"
#include "log/log.h"
#include "procname_common.h"
#include "parameter.h"

// 全局变量，用于重启时调用
int (*server_func)(const char *, PARAMENT *config) = NULL;
const char *server_home = NULL;

//创建多进程
void
runMutilProcessServer(const char *home, PARAMENT * config, Func func){
	printLog("%d > 设置主进程名称\n", getpid());
	//设置主进程名
    set_process_name("minhd");
	

	printLog("%d > 启动多进程服务器\n", getpid());
    
    // 保存函数指针和参数，用于重启
    server_func = func;
    server_home = home;


    // 设置信号处理器
    printLog("%d > 设置信号处理...\n", getpid());
    setupSignalHandlers();
    
    // 创建工作进程
    printLog("%d > 创建 %d 个工作进程\n", getpid(), MAX_PROCESS);
    for(int i = 0; i < MAX_PROCESS; i++) {
        pid_t pid = fork();
        if(pid == -1) {
            fprintf(stderr, "创建进程失败: %s\n", strerror(errno));
            continue;
        }
        if(pid == 0){
        	printLog("%d > 设置工作进程名称\n", getpid());
        	//设置工作进程名
    		set_process_name_fmt("worker-%d", i);
            //子进程
            printLog("%d > 工作进程 %d 启动\n", getpid(), i); 
            //重置信号处理（子进程不需要处理这些信号）
            signal(SIGCHLD, SIG_DFL);
            signal(SIGINT,  SIG_DFL);
            signal(SIGTERM, SIG_DFL);
            // 执行服务器函数
            func(home, config);
            // 退出子进程
            exit(0);
        } else {
            // 父进程记录子进程ID
            worker_pids[i] = pid;
            printLog("%d > 创建工作进程 %d (PID: %d)\n", getpid(), i, pid);
            usleep(100000);  // 短暂延迟，避免同时创建太多进程
        }
    }
    
    // 父进程主循环（等待优雅关闭）
    printLog("%d > 服务器运行中，按 Ctrl+C 停止...\n", getpid());
    while(server_running){
        //主进程可以在这里执行监控任务 例如：健康检查、配置重载、日志轮转等
        
        //定期检查工作进程状态
        static int check_count = 0;
        if(++check_count % 10 == 0){  // 每10秒检查一次
            printLog("%d > 健康检查: ", getpid());
            for(int i = 0; i < MAX_PROCESS; i++){
                if (worker_pids[i] > 0) {
                    printLog("[%d:%d] ", i, worker_pids[i]);
                } else {
                    printLog("[%d:---] ", i);
                }
            }
            printLog("\n");
        }
        sleep(1);  // 短暂休眠
    }
    // 等待所有子进程结束
    printLog("%d > 等待所有工作进程退出...\n", getpid());
    int timeout = 30;  // 30秒超时
    while(timeout-- > 0){
        int all_dead = 1;
        for (int i = 0; i < MAX_PROCESS; i++) {
            if (worker_pids[i] > 0) {
                all_dead = 0;
                break;
            }
        }
        if (all_dead) {
            break;
        }
        sleep(1);
    }
    // 强制终止剩余进程
    if(timeout <= 0) {
        printLog("%d > 超时，强制终止剩余进程\n", getpid());
        for (int i = 0; i < MAX_PROCESS; i++) {
            if (worker_pids[i] > 0) {
                kill(worker_pids[i], SIGKILL);
            }
        }
    }
    //等待所有进程结束
    while (waitpid(-1, NULL, 0) > 0) {
        // 继续等待
    }
    printLog("%d > 服务器已关闭\n", getpid());
}
