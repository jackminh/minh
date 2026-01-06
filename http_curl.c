#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <curl/curl.h>
#include "http_curl.h"

// 回调函数，将下载的数据写入文件
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream){
    return fwrite(ptr, size, nmemb, stream);
}

int download_and_open(const char *url, const char *local_path) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    // 打开本地文件用于写入
    fp = fopen(local_path, "wb");
    if (!fp) {
        perror("fopen failed");
        return -1;
    }
    
    // 初始化 curl
    curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        return -1;
    }
    
    // 设置 curl 选项
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // 执行下载
    res = curl_easy_perform(curl);
    
    // 清理
    curl_easy_cleanup(curl);
    fclose(fp);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", 
                curl_easy_strerror(res));
        return -1;
    }
    // 现在可以用 open() 打开本地文件
    int fd = open(local_path, O_RDONLY);
    return fd;
}