#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include "server.h"
#include "process.h"
#include "log/log.h"
#include "parameter.h"
#include "select/select.h"

//主体和端口配置
PARAMENT config = {0};

int 
main(int argc, char **argv){
	printLog("%d > 开始解析命令行参数\n", getpid());
	if(parseCommandParaments(argc, argv, &config) != 0){
		return -1;
	}
	//运行多进程服务器
	const char * home = "./www";
	//初始化服务器
	if(initServer(&config)){
		exit(EXIT_FAILURE);
	}
	//运行服务器
	runMutilProcessServer(home,&config,runServer);
	//终结服务器
	deinitServer(&config);

	return EXIT_SUCCESS;
}
