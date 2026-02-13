#ifndef __SERVER_H__
#define __SERVER_H__

#include "parameter.h"

//初始化服务器
int initServer(PARAMENT *config);

//运行服务器
int runServer(const char * home, PARAMENT *config);

//终结服务器
void deinitServer(PARAMENT *config);

#endif