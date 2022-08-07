#include "bluetooth.h"
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs_init.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "time.h"
#include "sys/time.h"

#define SPP_SERVER_NAME "PANTRY_IO_SERVER"
#define DEVICE_NAME "PANTRY_IO_BT"
#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA

#define USERNAME_PREFIX "username{"
#define USERNAME_POST "}username"

#define PASSWD_PREFIX "password{"
#define PASSWD_POST "}password"

#define AUTH_KEY_PREFIX "authkey{"
#define AUTH_KEY_POST "}authkey"

#define ID_PREFIX "id{"
#define ID_POST "}id"

#define BLUETOOTH_DATA_QUERY_TIME 120

#define BLUETOOTH_MSG_MAX_LEN 304

SemaphoreHandle_t bluetooth_semaphore = NULL;
static volatile u_char bluetooth_written = 0;
static char bluetooth_msg[BLUETOOTH_MSG_MAX_LEN] = {0};
static bool bluetooth_used = false;

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static struct timeval time_old;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

static char *bda2str(uint8_t * bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
    case ESP_SPP_INIT_EVT:
        if (param->init.status == ESP_SPP_SUCCESS) {
            ESP_LOGI(__func__, "ESP_SPP_INIT_EVT");
            esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
        } else {
            ESP_LOGE(__func__, "ESP_SPP_INIT_EVT status:%d", param->init.status);
        }
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(__func__, "ESP_SPP_CLOSE_EVT status:%d handle:%d close_by_remote:%d", param->close.status,
                 param->close.handle, param->close.async);
        break;
    case ESP_SPP_START_EVT:
        if (param->start.status == ESP_SPP_SUCCESS) {
            ESP_LOGI(__func__, "ESP_SPP_START_EVT handle:%d sec_id:%d scn:%d",
                     param->start.handle, param->start.sec_id,
                     param->start.scn);

            esp_bt_dev_set_device_name(DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else {
            ESP_LOGE(__func__, "ESP_SPP_START_EVT status:%d", param->start.status);
        }
        break;
    case ESP_SPP_DATA_IND_EVT:
        ESP_LOGI(__func__, "ESP_SPP_DATA_IND_EVT len:%d handle:%d",
                 param->data_ind.len, param->data_ind.handle);
        if (param->data_ind.len < BLUETOOTH_MSG_MAX_LEN) {
            strcpy(bluetooth_msg, (char *) param->data_ind.data);
            bluetooth_written = 1;
        }
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(__func__, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%d, rem_bda:[%s]", param->srv_open.status,
                 param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
        gettimeofday(&time_old, NULL);
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char bda_str[18] = {0};

    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(__func__, "authentication success: %s bda:[%s]",
                        param->auth_cmpl.device_name,
                        bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
            } else {
                ESP_LOGE(__func__, "authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;

        case ESP_BT_GAP_PIN_REQ_EVT:
            ESP_LOGI(__func__, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
            if (param->pin_req.min_16_digit) {
                ESP_LOGI(__func__, "Input pin code: 0000 0000 0000 0000");
                esp_bt_pin_code_t pin_code = {0};
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
            } else {
                ESP_LOGI(__func__, "Input pin code: 1234");
                esp_bt_pin_code_t pin_code;
                pin_code[0] = '1';
                pin_code[1] = '2';
                pin_code[2] = '3';
                pin_code[3] = '4';
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            }
            break;

        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(__func__, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;

        case ESP_BT_GAP_MODE_CHG_EVT:
            ESP_LOGI(__func__, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]",
                    param->mode_chg.mode,
                    bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
            break;

        default:
            break;
    }
}

char* parse_string(const char* string, const char* prefix, const char* ender)
{
    char* str_start = strstr(string, prefix);
    char* str_end = strstr(string ,ender);

    if (str_start == NULL || str_end == NULL) {
        return NULL;
    }

    str_start += strlen(USERNAME_PREFIX);
    char* outstring = malloc(sizeof(char) * MAX_USERNAME_LENGTH);
    char* outstring_start = outstring;
    while (str_start < str_end) {
        *outstring = *str_start;
        ++str_start;
        ++outstring;
    }

    *outstring = '\0';
    return outstring_start;

}

char* parse_username(const char* string)
{
    return parse_string(string, USERNAME_PREFIX, USERNAME_POST);
}

char* parse_password(const char* string)
{
    return parse_string(string, PASSWD_PREFIX, PASSWD_POST);
}

char* parse_authkey(const char* string)
{
    return parse_string(string, AUTH_KEY_PREFIX, AUTH_KEY_POST);
}

char* parse_id(const char* string)
{
    return parse_string(string, ID_PREFIX, ID_POST);
}

void vSaveBluetoothCredientials(void *parameters)
{
    unsigned repeats = 0;
    bool wifi_set = false;
    bool auth_token_set = false;
    bool id_set = false;

    while (repeats < BLUETOOTH_DATA_QUERY_TIME &&
           !(wifi_set && auth_token_set && id_set)) {

        repeats++;

        if (bluetooth_written) {
            bluetooth_written = 0;
            char* username = parse_username(bluetooth_msg);
            char* passwd = parse_password(bluetooth_msg);
            char* id = parse_id(bluetooth_msg);

            if (username != NULL && passwd != NULL) {
                save_wifi_credientials(username, passwd);
                wifi_set = true;
            }
            char* auth_token = parse_authkey(bluetooth_msg);
            if (auth_token != NULL) {
                save_auth_token(auth_token);
                auth_token_set = true;
            }
            if (id != NULL) {
                save_unit_id(id);
                id_set = true;
            }

            free(auth_token);
            free(username);
            free(passwd);
            free(id);
            memset(bluetooth_msg, 0, BLUETOOTH_MSG_MAX_LEN);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    deinit_bluetooth();
    vTaskDelete(NULL);
}

void deinit_bluetooth(void)
{
    esp_err_t ret = 0;
    if ((ret = esp_spp_deinit()) != ESP_OK) {
        ESP_LOGE(__func__, "spp_deinit_failed: %s\n",esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bluedroid_disable()) != ESP_OK) {
        ESP_LOGE(__func__, "esp_bluedroid_disable: %s\n", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bluedroid_deinit()) != ESP_OK) {
        ESP_LOGE(__func__, "esp_bluedroid_deinit: %s\n", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bt_controller_disable()) != ESP_OK) {
        ESP_LOGE(__func__, "esp_bt_controller_disable: %s\n", esp_err_to_name(ret));
        return;
    }
    if ((ret = esp_bt_controller_deinit()) != ESP_OK) {
        ESP_LOGE(__func__, "esp_bt_controller_disable: %s\n", esp_err_to_name(ret));
        return;
    }
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
}

void init_bluetooth(void)
{
    // The bluetooth init can only be done once for some reason.
    if (bluetooth_used) {
        return;
    }

    esp_err_t ret = 0;
    if  (esp_bt_controller_get_status() ==  ESP_BT_CONTROLLER_STATUS_ENABLED) {
        return;
    }

    char bda_str[18] = {0};

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(__func__, "initialize controller failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(__func__, "enable controller failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(__func__, "initialize bluedroid failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(__func__, "enable bluedroid failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(__func__, "gap register failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(__func__, "spp register failed: %s\n", esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(__func__, "spp init failed: %s\n", esp_err_to_name(ret));
        return;
    }

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
    bluetooth_used = true;

    ESP_LOGI(__func__, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
    xTaskCreate(vSaveBluetoothCredientials, "bt_task", 2048, NULL, 10, NULL);

}
