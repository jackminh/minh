#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "conf_parse.h"
ConfigEntry *config_entries = NULL;
// 去除字符串首尾空白
static void trim(char *str) {
    char *end;
    
    // 去除首部空白
    while (isspace((unsigned char)*str)) str++;
    
    // 如果是空字符串
    if (*str == 0) return;
    
    // 去除尾部空白
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // 写入新的终止符
    *(end + 1) = 0;
}

// 解析配置文件
int parse_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("无法打开配置文件");
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    char current_section[MAX_SECTION_LENGTH] = "global";
    int line_num = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        char *ptr = line;
        
        // 去除行尾换行符
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // 跳过前导空白
        while (isspace((unsigned char)*ptr)) ptr++;
        
        // 跳过空行和注释
        if (*ptr == '\0' || *ptr == '#' || *ptr == ';') {
            continue;
        }
        
        // 检查是否是节头 [section]
        if (*ptr == '[') {
            char *end = strchr(ptr, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, ptr + 1, sizeof(current_section) - 1);
                trim(current_section);
            }
            continue;
        }
        
        // 解析键值对
        char *equals = strchr(ptr, '=');
        if (equals) {
            *equals = '\0';
            char *key = ptr;
            char *value = equals + 1;
            
            trim(key);
            trim(value);
            
            // 去除值中的引号
            if (value[0] == '"' && value[strlen(value)-1] == '"') {
                value[strlen(value)-1] = '\0';
                memmove(value, value + 1, strlen(value));
            }
            
            // 保存配置项
            ConfigEntry *entry = malloc(sizeof(ConfigEntry));
            if (!entry) continue;
            
            strncpy(entry->section, current_section, sizeof(entry->section)-1);
            strncpy(entry->key, key, sizeof(entry->key)-1);
            strncpy(entry->value, value, sizeof(entry->value)-1);
            entry->next = config_entries;
            config_entries = entry;
            
            // 调试输出
            //printf("配置: [%s] %s = %s\n", current_section, key, value);
        }
    }
    
    fclose(file);
    return 0;
}

// 获取配置值
const char *get_config_string(const char *section, const char *key, const char *default_value) {
    ConfigEntry *entry = config_entries;
    while (entry) {
        if(strcmp(entry->section, section) == 0 && 
            strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return default_value;
}

int get_config_int(const char *section, const char *key, int default_value) {
    const char *value = get_config_string(section, key, NULL);
    if (value) {
        return atoi(value);
    }
    return default_value;
}

double get_config_double(const char *section, const char *key, double default_value) {
    const char *value = get_config_string(section, key, NULL);
    if (value) {
        return atof(value);
    }
    return default_value;
}

int get_config_bool(const char *section, const char *key, int default_value) {
    const char *value = get_config_string(section, key, NULL);
    if (value) {
        if (strcasecmp(value, "true") == 0 || 
            strcasecmp(value, "yes") == 0 || 
            strcasecmp(value, "1") == 0) {
            return 1;
        }
        if (strcasecmp(value, "false") == 0 || 
            strcasecmp(value, "no") == 0 || 
            strcasecmp(value, "0") == 0) {
            return 0;
        }
    }
    return default_value;
}

// 清理配置内存
void cleanup_config() {
    ConfigEntry *entry = config_entries;
    while (entry) {
        ConfigEntry *next = entry->next;
        free(entry);
        entry = next;
    }
    config_entries = NULL;
}



