//参数模块
#ifndef __PARMMETER_H__
#define __PARMMETER_H__

//参数输出
typedef struct {
	char* host;
	int port;
	char *mode ;  //使用哪种I/O模式
}PARAMENT;


//解析命令行参数,返回解析后的参数结构体
int
parseCommandParaments(int argc, char **argv, PARAMENT *config);

//打印输出帮助
void 
usage(const char *program_name);


#endif