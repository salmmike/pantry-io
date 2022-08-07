#include <stdio.h>
#include "http.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "nvs_init.h"

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(__func__, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(__func__, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                ESP_LOG_BUFFER_HEX(__func__, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(__func__, "HTTP_EVENT_DISCONNECTED");

            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(__func__, "Last esp error code: 0x%x", err);
                ESP_LOGI(__func__, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(__func__, "HTTP_EVENT_REDIRECT");
            break;
        default:
            ESP_LOGD(__func__, "Other http event: %d", evt->event_id);
            return ESP_FAIL;
    }
    return ESP_OK;
}


int http_request(const char* host, const char* path, char *response_data, esp_http_client_method_t method, char *data, bool use_auth)
{
    /**
     * Sends HTTPS request to host/path with authorization header
     * Response from server is saved in response_data. 
     *
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
    printf("Start request\n");
    esp_err_t err = 0;
    int return_code = 0;
    esp_http_client_config_t config = {
        .host = host,
        .path = path,
        .event_handler = _http_event_handler,
        .user_data = response_data,        // Pass address of local buffer to get response
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    printf("Init client\n");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    err = esp_http_client_set_method(client, method);

    printf("esp client set method: %d\n", err);
    printf("Set headers:\n");
    err = esp_http_client_set_header(client, "Content-Type", "application/json");
    char* token_header = NULL;
    char* auth_token = NULL;
    if (use_auth) {
        printf("Set auth header:\n");
        token_header = calloc(64, sizeof(char));
        auth_token = get_auth_token();
        sprintf(token_header, "Bearer %s", auth_token);
        err = esp_http_client_set_header(client, "Authorization", token_header);
    }
    printf("Headers set %d\n", err);

    if (method == HTTP_METHOD_POST) {
        printf("Set post field %d \n", err);
        err = esp_http_client_set_post_field(client, data, strlen(data));

    }
    printf("Perform action");
    printf("Host: %s, path: %s, data: %s, token: %s\n", host, path, data, token_header);

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(__func__, "HTTP Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(__func__, "HTTP request failed: %s", esp_err_to_name(err));
        return_code = 1;
    }

    esp_http_client_cleanup(client);
    printf("HTTP done\n");
    free(token_header);
    free(auth_token);
    printf("Free token header\n");
    return return_code;
 
}

int http_get(const char* host, const char* path, char* get_response, bool use_auth)
{
    return http_request(host, path, get_response, HTTP_METHOD_GET, NULL, use_auth);
}

int http_post(const char* host, const char* path, char *post_response, char* data, bool use_auth)
{
    return http_request(host, path, post_response, HTTP_METHOD_POST, data, use_auth);
}
