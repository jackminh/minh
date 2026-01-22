#ifndef __SIGNALS_H__
#define __SIGNALS_H__
//信号处理函数 

#define MAX_PROCESS 4

extern pid_t worker_pids[MAX_PROCESS];
extern volatile sig_atomic_t server_running;

// 信号处理函数声明
void sigchldHandler(int sig);
void setupSignalHandlers(void);
// 重启工作进程
void restartWorker(int worker_id);
//SIGINT/SIGTERM 信号处理函数
void 
shutdownHandler(int sig);

#endif