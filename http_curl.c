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

// int main() {
//     const char *url = "https://stevens.netmeister.org/631/f25-hw1.html";
//     const char *local_file = "./home/f25-hw1.html";
    
//     int fd = download_and_open(url, local_file);
//     if (fd >= 0) {
//         printf("文件下载成功，fd = %d\n", fd);
        
//         // 读取文件内容
//         char buffer[1024];
//         ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
//         if (n > 0) {
//             buffer[n] = '\0';
//             printf("内容：%s\n", buffer);
//         }
//         close(fd);
//        // unlink(local_file);  // 删除临时文件
//     }
    
//     return 0;
// }