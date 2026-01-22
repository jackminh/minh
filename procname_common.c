#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "procname_common.h"
// 平台特定的实现
#ifdef __APPLE__
#include <stdlib.h>  // getprogname/setprogname

int set_process_name(const char *name) {
    if (!name) return -1;
    setprogname(name);
    return 0;
}

const char *get_process_name(void) {
    return getprogname();
}

#elif defined(__linux__)
#include <sys/prctl.h>

int 
set_process_name(const char *name) {
    if (!name) return -1;
    
    // 使用 prctl
    if (prctl(PR_SET_NAME, name, 0, 0, 0) < 0) {
        fprintf(stderr,"prctl error: %s\n",strerror(errno));
        return -1;
    }

    return 0;
}

const char *
get_process_name(void) {

    static char name[256] = {0};
    
    // 从 /proc 获取
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", getpid());
    
    FILE *f = fopen(path, "r");
    if (f) {
        if (fgets(name, sizeof(name), f)) {
            name[strcspn(name, "\n")] = '\0';
        }
        fclose(f);
        return name;
    }
    
    return "unknown";
}

#else
// 其他平台使用 argv[0]

int set_process_name(const char *name) {
    if (!name) return -1;
    
    extern char **__argv;
    if (__argv && __argv[0]) {
        strncpy(__argv[0], name, strlen(__argv[0]));
        return 0;
    }
    
    return -1;
}

const char *get_process_name(void) {
    extern char **__argv;
    if (__argv && __argv[0]) {
        return __argv[0];
    }
    return "unknown";
}

#endif

// 通用格式化函数
int set_process_name_fmt(const char *fmt, ...) {
    char name[1024];
    va_list args;
    
    va_start(args, fmt);
    int len = vsnprintf(name, sizeof(name), fmt, args);
    va_end(args);
    
    if (len <= 0) {
        return -1;
    }
    
    return set_process_name(name);
}







