#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "process.h"
#include "log/log.h"


int 
main(int argc, char **argv){
	//初始化服务器
	if(initServer(argc,argv)){
		exit(EXIT_FAILURE);
	}
	//运行多进程服务器
	const char * home = "./www";
	runMutilProcessServer(home,runServer);
	
	//终结服务器
	deinitServer();

	return EXIT_SUCCESS;
}
