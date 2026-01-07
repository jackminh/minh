#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "comm_socket.h"
#include "comm_func.h"




int set_worker_process_name(char **argv, const char *name, int is_master, int i){
	if (argv == NULL || argv[0] == NULL || name == NULL) {
        return -1;
    }
    char new_name[32] = {};
    if(is_master == 1){//设置主进程的名称
        snprintf(new_name, sizeof(new_name), "%s-master", name);
    }else{//设置工作进程名称
        snprintf(new_name, sizeof(new_name),"%s-worker-%d", name, i);
    }

    size_t src_len      = strlen(new_name);
    size_t dst_len      = strlen(argv[0]);

    if(dst_len >= ( src_len + 1 )){
    	strcpy(argv[0], name);  //直接copy name to argv[0],包括'\0'
    }else{
        snprintf(argv[0], sizeof(new_name), "%s" ,new_name);
    }
	return 0;
}

char * clear_string_head_tail_space(const char *str){
    if(str == NULL || *str == '\0') {
        printf("(empty string)\n");
        return NULL;
    }
    size_t len = strlen(str);
    if (len == 0) {
        printf("(empty string)\n");
        return NULL;
    }
    const char *start = str;  //非空白字符的起始地址
    while( *start && isspace((unsigned char)*start) ) {
        start++;
    }
    // 如果全是空白字符
    if(*start == '\0') {
        printf("(all spaces)\n");
        return NULL;
    }
    const char *end =  str + strlen(str) - 1;   //非空白字符的结束地址
    while( end > start && isspace((unsigned char)*end)){
        end--;
    }

    // 计算新长度并移动字符串
    size_t new_len = end - start + 1;
    char *result = (char *) malloc(new_len + 1);
    if (result == NULL) return NULL;

    memcpy(result, start, new_len);
    result[new_len] = '\0';

    return result;
    
}
