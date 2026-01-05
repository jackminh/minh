#ifndef __INIT_SIGNAL_H
#define __INIT_SIGNAL_H


extern volatile sig_atomic_t stop_server;
extern volatile sig_atomic_t reload_config;

void setup_worker_signals(void);

// SIGPIPE 处理器 - 忽略 Broken Pipe
void sigpipe_handler(int sig);
// SIGHUP 处理器 - 重载配置
void sighup_handler(int sig);
// SIGTERM/SIGINT 处理器 - 优雅关闭
void sigterm_handler(int sig);
// SIGCHLD 处理器 - 回收僵尸进程
void sigchld_handler(int sig);

//设置所有信号处理器
void 
setup_signals(void);


#endif
