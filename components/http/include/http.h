// Methods for HTTP communication.
#include <stdbool.h> 

#define MAX_HTTP_OUTPUT_BUFFER 4096

int http_post(const char* host, const char* path, char *post_response, char* data, bool use_auth);
int http_get(const char* host, const char* path, char *get_response, bool use_auth);
