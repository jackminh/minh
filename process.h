#ifndef __PROCESS_H__
#define __PROCESS_H__

//运行多进程服务器
void
runMutilProcessServer(const char *home, int (*func)(const char * home));


#endif