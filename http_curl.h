#ifndef __HTTP_CURL_H__
#define __HTTP_CURL_H__


size_t 
write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

int 
download_and_open(const char *url, const char *local_path);




#endif