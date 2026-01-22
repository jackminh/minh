#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "log/log.h"

#include "parameter.h"

//打印输出帮助
void 
usage(const char *program_name){
	fprintf(stderr, "用法: %s[选项]\n", program_name);
	fprintf(stderr, "选项:\n");
	fprintf(stderr, "  -h, --help				 	显示此帮助信息\n");
	fprintf(stderr, "  -a, --address <IP>				绑定IP地址(必需)\n");
	fprintf(stderr, "  -p, --port <PORT>      			绑定端口号(必需)\n");
    fprintf(stderr, "示例:\n");
    fprintf(stderr, "  %s -p 8080 -a 127.0.0.1\n",program_name);
    fprintf(stderr, "  %s --port 8080 --address 127.0.0.1\n", program_name);
}

//校验解析出来的参数是否有效
static int 
validateArgs(PARAMENT *config){
	char* host = config->host;
    struct sockaddr_in sa;
    if(inet_pton(AF_INET, host, &(sa.sin_addr)) ==1){
        return 0;  
    }else if(strcmp(host,"127.0.0.1") == 0){
    	return 0;
    }else if(strcmp(host,"localhost") == 0){
    	return 0;
    }else{
    	return -1;
    }
	return 0;
}

//解析命令行参数
int
parseCommandParaments(int argc, char **argv, PARAMENT *config)
{
	memset(config,0,sizeof(PARAMENT));  //初始化配置中host为NULL,port为0
	if(argc == 1){
		config->host = strdup("0.0.0.0");
		config->port = 9527;
		return 0;
	}
	const struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "port", required_argument, NULL, 'p' },
		{ "address", required_argument, NULL,'a' },
		{0, 0, 0, 0}
	};
	int ch;
	char* endptr;
	long port;
    while((ch=getopt_long(argc, argv, "hp:a:",longopts, NULL)) != -1){
    	switch(ch){
    	case 'h':
    		usage(argv[0]);
    		break;
    	case 'p':
    		port = strtol(optarg,&endptr,10);  //将给定的optarg,一位一位按10进制转换,最终endptr 二级指针指向末尾('\0')
    		if( *endptr !='\0' || port <=0 || port > 65535){
    			fprintf(stderr,"无效端口号: %s\n",optarg);
    		}
    		config->port = (int) port;   //端口
    		break;
    	case 'a':
			if(config->host != NULL){
    			free(config->host);
    		}
    		config->host = strdup(optarg); //主机
    		if(config->host == NULL){
    			fprintf(stderr,"内存分配置失败\n");
    			return -1;
    		}
    		break;
    	default:
    		usage(argv[0]);
    	}
	}

	if(optind < argc){
		while(optind < argc){
			fprintf(stderr,"无效参数: %s",argv[optind++]);
		}
		fprintf(stderr,"\n");
	}
	if(validateArgs(config)!=0){
		fprintf(stderr,"参数输入错误: %s\n",config->host);
		usage(argv[0]);
		return -1;
	}
	return 0;
}

