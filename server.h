#ifndef __SERVER_H__
#define __SERVER_H__

//初始化服务器
int initServer(int argc, char **argv);

//运行服务器
int runServer(const char * home);

//终结服务器
void deinitServer(void);

#endif