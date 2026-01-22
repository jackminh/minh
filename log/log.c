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
    
    char *d_env = getenv("DEBUG");
    if (!d_env) {
        return debug;
    }
    
    // 计算需要的缓冲区大小
    size_t len = strlen(d_env);
    if (len == 0) {
        return debug;
    }
    
    // 分配足够的内存（包括终止符）
    char *debug_env = (char *)malloc(len + 1);
    if (!debug_env) {
        perror("内存分配失败");
        return debug;
    }
    
    // 复制并转换为小写
    for (size_t i = 0; i < len; i++) {
        debug_env[i] = tolower(d_env[i]);
    }
    debug_env[len] = '\0';
    // 检查布尔值
    if (strcmp(debug_env, "true") == 0 ||
        strcmp(debug_env, "1") == 0 ||
        strcmp(debug_env, "yes") == 0 ||
        strcmp(debug_env, "on") == 0) {
        debug = 1;
    }
    free(debug_env);
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