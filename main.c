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
	if(strcasecmp(config.mode,"blocking")==0){       //默认阻塞模式
		//初始化服务器
		if(initServer(&config)){
			exit(EXIT_FAILURE);
		}
		runMutilProcessServer(home,runServer);
	}else if(strcasecmp(config.mode, "select") == 0){  //select 模式
		//初始化服务器
		if(initSelectServer(&config)){
			exit(EXIT_FAILURE);
		}
		runMutilProcessServer(home,runSelectServer);
	}else if(strcasecmp(config.mode, "epoll") == 0){   //epoll模式
		//todo
		printf("epoll\n");
	}
	//终结服务器
	deinitServer(&config);

	return EXIT_SUCCESS;
}
