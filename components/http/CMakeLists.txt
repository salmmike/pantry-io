idf_component_register(SRCS "http.c"
                       INCLUDE_DIRS "include"
                       INCLUDE_DIRS "../nvs_init/include"
                       REQUIRES esp_http_client
                       REQUIRES esp_https_server
                       REQUIRES esp_event
                       REQUIRES esp_netif)
