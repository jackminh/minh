#include <stdio.h>
#include <stdarg.h>  // 必须包含这个头文件
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
// 可变参数函数需要三个步骤：
// 1. 使用 va_list 类型定义参数列表
// 2. 使用 va_start 初始化参数列表
// 3. 使用 va_arg 访问参数
// 4. 使用 va_end 清理参数列表

static int 
getDebugModeSafe(){
    int debug = 0;
    char *debug_env = getenv("DEBUG");
    if(!debug_env){
        return debug;
    }
    //比较
    if(strcasecmp(debug_env,"true") == 0
      || strcasecmp(debug_env,"yes") == 0
      || strcasecmp(debug_env,"1") == 0
      || strcasecmp(debug_env,"enable")==0
      || strcasecmp(debug_env,"on") == 0)
    {
        debug = 1;
    }

    return debug;

}


void 
printLog(const char *format, ...){
    int debug = getDebugModeSafe();
    if (debug) {
        // 1. 定义参数列表
        va_list args;
        
        // 2. 初始化参数列表
        // format 是最后一个固定参数
        va_start(args, format);
        
        // 3. 使用 vprintf 打印可变参数
        vprintf(format, args);
        
        // 4. 清理参数列表
        va_end(args);
    } else {
        (void)format;  // 消除未使用警告
    }
}