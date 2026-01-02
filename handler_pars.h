#ifndef __HANDLER_PARS_H
#define __HANDLER_PARS_H
/**
 * 
 * @param  argc [description]
 * @param  argv [description]
 * @return      [description]
 */
void handler_pars(int argc, char ** argv_ptr, server_addr_config *config);
/**
 * @brief 打印使用说明
 */
void print_usage(const char *program_name);

/**
 * @param config  服务配置
 * @return int 有效返回0，无效返回-1
 */
int validate_args(server_addr_config * config);

/**
 * [free_config description]
 * @param config [description]
 */
void free_config(server_addr_config *config);

/**
 * init config
 * @param config [description]
 */
void init_config(server_addr_config *config);

#endif
