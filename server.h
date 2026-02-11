#ifndef __SERVER_H__
#define __SERVER_H__

#include "parameter.h"

//初始化服务器
int initServer(PARAMENT *config);

//运行服务器(阻塞格式)
int runServer(const char * home);


//初始化select模式服务器
int initSelectServer(PARAMENT *config);
//运行服务器(非阻塞格式)
int runSelectServer(const char * home);




//终结服务器
void deinitServer(PARAMENT *config);

#endif