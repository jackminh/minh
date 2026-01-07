#ifndef __COMM_FUNC_H__
#define __COMM_FUNC_H__

//设置进程名称
int set_worker_process_name(char **argv, const char *name, int is_master, int i);


//清空首尾空格
char * clear_string_head_tail_space(const char *str);

#endif