#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "nvs_init.h"
#include "wifi.h"
#include "http.h"
#include "button-states.h"

void app_main(void)
{
    init_nvs();
    fast_scan();
    init_gpio();
    init_state_sender();
}
