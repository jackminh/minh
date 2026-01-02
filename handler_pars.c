#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "comm_socket.h"
#include "handler_pars.h"

void 
handler_pars(int argc, char ** argv,server_addr_config *config){
	/* 定义长选项 */
    static struct option long_options[] = {
        {"help",        no_argument,       0, 'h'},
        {"address",     required_argument, 0, 'a'},
        {"port",        required_argument, 0, 'p'},
        {"daemon",      no_argument,       0, 'd'},
        {"verbose",     no_argument,       0, 'v'},
        {"config",      required_argument, 0, 'c'},
        {"max-conn",    required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "ha:p:dvc:m:", 
                              long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':  /* 帮助 */
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'a':  /* 地址 */
                config->host = strdup(optarg);
                break;
                
            case 'p':  /* 端口 */
                config->port = strdup(optarg);
                break;
                
            case 'd':  /* 守护进程模式 */
                config->daemon_mode = 1;
                break;
                
            case 'v':  /* 详细模式 */
                config->verbose = 1;
                break;
                
            case 'c':  /* 配置文件 */
                config->config_file = strdup(optarg);
                break;
                
            case 'm':  /* 最大连接数 */
                config->max_connections = atoi(optarg);
                break;
                
            case '?':  /* 未知选项 */
                fprintf(stderr, "使用 -h 或 --help 查看帮助信息\n");
                exit(EXIT_FAILURE);
                
            default:
                fprintf(stderr, "未知错误\n");
                exit(EXIT_FAILURE);
        }
    }
    
    /* 检查是否有额外的参数 */
    if (optind < argc) {
        fprintf(stderr, "警告: 忽略额外参数: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
    }

}


void
free_config(server_addr_config *config){
    if(config->host != NULL) {
        free(config->host);
        config->host = NULL;
    }
    if(config->port != NULL) {
        free(config->port);
        config->port = NULL;
    }
    if(config->config_file != NULL){
    	free(config->config_file);
    	config->config_file=NULL;
    }
}

    


void 
print_usage(const char *program_name){
	fprintf(stderr, "\n用法: %s[选项]\n\n", program_name);
	fprintf(stderr, "选项:\n");
	fprintf(stderr, "  -h, --help				 	显示此帮助信息\n");
	fprintf(stderr, "  -a, --address <IP>				绑定IP地址(默认:0.0.0.0)\n");
	fprintf(stderr, "  -p, --port <PORT>      			绑定端口号 (必需)\n");
    fprintf(stderr, "  -d, --daemon           			以守护进程模式运行\n");
    fprintf(stderr, "  -v, --verbose          			详细输出模式\n");
    fprintf(stderr, "  -c, --config <FILE>    			指定配置文件\n");
    fprintf(stderr, "  -m, --max-conn <NUM>   			最大连接数 (默认: 1024)\n\n");
    fprintf(stderr, "示例:\n");
    fprintf(stderr, "  %s -p 8080 -a 127.0.0.1\n",program_name);
    fprintf(stderr, "  %s --port 8080 --daemon --verbose\n", program_name);
    fprintf(stderr, "  %s -p 80 -c /etc/server.conf -m 2048\n\n", program_name);
}

int 
validate_args(server_addr_config * config){
	if(config->port != NULL){
		char *endptr;
		long port = strtol(config->port,&endptr,10);
		if(*endptr !='\0'){
			fprintf(stderr, "错误: 端口号必须是数字\n");
            return -1;
		}
		if(port < 1 || port > 65535){
			fprintf(stderr, "错误: 端口号必须在 1-65535 范围内\n");
            return -1;
		}
	}else{
		return -1;
	}
	/* 验证最大连接数 */
    if (config->max_connections < 1 || config->max_connections > 65535) {
        fprintf(stderr, "错误: 最大连接数必须在 1-65535 范围内\n");
        return -1;
    }
    if(config->config_file !=NULL){
    	if(access(config->config_file,R_OK)!=0){
    		fprintf(stderr, "错误: 无法读取配置文件 '%s'\n", config->config_file);
            return -1;
    	}
    }

	return 0;
}

void 
init_config(server_addr_config *config){
	if(config->host == NULL){
		config->host = strdup("0.0.0.0");
	}
	if(config->port == NULL){
		config->port = strdup("8080");
	}
	config->daemon_mode = 0;
	config->verbose = 0;
	config->max_connections = 1024;
}

