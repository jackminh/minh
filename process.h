#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "parameter.h"

typedef int (*Func)(const char *home, PARAMENT *config);

//运行多进程服务器
void
runMutilProcessServer(const char *home, PARAMENT *config ,Func func);


#endif