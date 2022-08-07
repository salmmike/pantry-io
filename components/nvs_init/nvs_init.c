#include <stdio.h>
#include "nvs.h"
#include "nvs_init.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#define DATA_STORAGE "data"
#define WIFI_PASSWD_KEY "wifi_passwd"
#define WIFI_USERNAME_KEY "wifi_network"
#define AUTH_TOKEN_KEY "auth_token"
#define ID_KEY "id_key"

#define NVS_LOG_TAG "NVS"

void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

char* get_wifi_passwd(void)
{
    return get_nvs_data(WIFI_PASSWD_KEY);
}

char* get_wifi_user(void)
{
    return get_nvs_data(WIFI_USERNAME_KEY);
}

char* get_auth_token()
{
    return get_nvs_data(AUTH_TOKEN_KEY);
}

char* get_unit_id(void)
{
    return get_nvs_data(ID_KEY);
}

char* get_nvs_data(const char* val_name)
{
    nvs_handle my_handle = 0;
    esp_err_t err = nvs_open(DATA_STORAGE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_LOG_TAG, "Failed to open nvs: %s\n", esp_err_to_name(err));
    }

    char* buffer = malloc(sizeof(char) * STR_LENGTH);
    size_t data_len = sizeof(char) * STR_LENGTH;
    err = nvs_get_str(my_handle, val_name, buffer, &data_len);

    switch (err) {
    case ESP_OK:
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        free(buffer);
        buffer = NULL;
        break;
    default :
        printf("Error reading saved data: %s\n", esp_err_to_name(err));
    }
    nvs_close(my_handle);
    if (buffer != NULL)
        printf("Buffer: %s\n", buffer);
    else
        printf("Buffer is null\n");
    return buffer;
}

int save_value(const char* value, const char* key)
{
    nvs_handle v_nvs_handle = 0;
    esp_err_t err = nvs_open(DATA_STORAGE, NVS_READWRITE, &v_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_LOG_TAG, "Failed to open nvs: %s\n", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(v_nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_LOG_TAG, "Failed to set nvs: %s\n", esp_err_to_name(err));
        return err;

    }
    nvs_close(v_nvs_handle);
    return ESP_OK;
}

int save_wifi_credientials(const char* wifi_name, const char* passwd)
{
    ESP_ERROR_CHECK(save_value(wifi_name, WIFI_USERNAME_KEY));
    ESP_ERROR_CHECK(save_value(passwd, WIFI_PASSWD_KEY));

    printf("Saved wifi network: %s\n", wifi_name);
    return 0;
}

int save_auth_token(const char* token)
{
    ESP_ERROR_CHECK(save_value(token, AUTH_TOKEN_KEY));
    printf("Saved auth token\n");
    return 0;
}

int save_unit_id(const char* id)
{
    ESP_ERROR_CHECK(save_value(id, ID_KEY));
    return 0;
}

