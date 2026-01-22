#ifndef __PROCNAME_COMMON_H__
#define __PROCNAME_COMMON_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 设置进程名
int set_process_name(const char *name);

// 获取进程名
const char *get_process_name(void);

// 设置带格式的进程名
int set_process_name_fmt(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif