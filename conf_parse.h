#ifndef __CONF_PARSE_H__
#define __CONF_PARSE_H__


//配置相关选项
#define MAX_LINE_LENGTH 1024
#define MAX_SECTION_LENGTH 128
#define MAX_KEY_LENGTH 128
typedef struct ConfigEntry {
    char section[MAX_SECTION_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_LINE_LENGTH];
    struct ConfigEntry *next;
} ConfigEntry;

int parse_config(const char *filename);
const char *get_config_string(const char *section, const char *key, const char *default_value);
int get_config_int(const char *section, const char *key, int default_value);
double get_config_double(const char *section, const char *key, double default_value);
int get_config_bool(const char *section, const char *key, int default_value);
void cleanup_config();

#endif