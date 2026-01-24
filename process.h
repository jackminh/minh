#ifndef __PROCESS_H__
#define __PROCESS_H__

typedef int (*Func)(const char *home);

//运行多进程服务器
void
runMutilProcessServer(const char *home, Func func);


#endif