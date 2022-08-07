#include "button-states.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "nvs_init.h"
#include "http.h"

static QueueHandle_t gpio_evt_queue = NULL;
static button_t *buttons;
static int buttons_size = 0;
static uint32_t send_time = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static uint32_t millis() {
    return esp_timer_get_time() / 1000;
}

static void refresh_buttons(void)
{
    for (int i=0; i<buttons_size; ++i) {
        buttons[i].state = (gpio_get_level(buttons[i].pin) == 0);
    }
}

static void send_state_task()
{
    while(1) {
        if (millis() > send_time) {
            refresh_buttons();
            char* button_state_json = get_button_state_json();
            char* post_response = calloc(MAX_HTTP_OUTPUT_BUFFER, sizeof(char));
            printf("Send to server: %s\n", button_state_json);
            http_post("pantry-io-api.herokuapp.com", "/db", post_response, button_state_json, true);
            ESP_LOGI("TAG", "POST data: %s", post_response);
            free(post_response);
            free(button_state_json);
            send_time = millis() + 1000*60*60;
        }
        vTaskDelay(1000);
    }
}



static void gpio_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            for (int i=0; i<=buttons_size; ++i) {
                if (buttons[i].pin != io_num) {
                    continue;
                }
                uint32_t time_between = (millis() - buttons[i].previous_time);
                if (time_between > BOUNCE_TIME_MS) {
                    bool current_state_pressed = (gpio_get_level(io_num) == 0);
                    buttons[i].state = current_state_pressed;
                    buttons[i].previous_time = millis();
                }
            }
            send_time = millis() + 5*1000;
        }
    }
}

void init_debouncer(uint64_t button_flag)
{
    uint8_t num_buttons = 0;

    for (int pin=0; pin<=39; pin++) {
        if ((1ULL<<pin) & button_flag) {
            num_buttons++;
        }
    }
    buttons_size = num_buttons;
    buttons = calloc(sizeof(button_t), num_buttons);

    int i = 0;
    for (int pin=0; pin<=39; pin++) {
        if ((1ULL<<pin) & button_flag) {
            buttons[i].pin = pin;
            buttons[i].state = (gpio_get_level(pin) == 0);
            printf("Init GPIO[%d] intr, val: %d\n", pin, gpio_get_level(pin));
            ++i;
        }
    }
}

char* get_button_state_json()
{
    char* json_list = malloc((2*buttons_size + 4 + 30) * sizeof(char));

    sprintf(json_list, "{\"unit_id\":\"%s\", \"items\":", get_unit_id());

    char* list_moder = json_list + strlen(json_list);

    *list_moder = '[';
    for (int i = 0; i < buttons_size; ++i) {
        ++list_moder;
        if (buttons[i].state) {
            *list_moder = '1';
        } else {
            *list_moder = '0';
        }
        ++list_moder;
        *list_moder = ',';
    }
    *list_moder = ']';
    ++list_moder;
    *list_moder = '}';
    ++list_moder;
    *list_moder = '\0';

    return json_list;
}

void init_input_buttons(uint64_t button_flag)
{
    init_debouncer(button_flag);

    for  (int pin=0; pin<=39; pin++) {
        uint64_t pin_flag = (1ULL<<pin);
        if (pin_flag & button_flag) {
            gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add(pin, gpio_isr_handler, (void*) pin);
        }
    }
}

void init_gpio()
{
    printf("Init gpio");
    gpio_config_t io_conf = {};

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL<<GPIO_INPUT_IO_0 | 1ULL<<GPIO_INPUT_IO_1);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    init_input_buttons(io_conf.pin_bit_mask);
}

void init_state_sender()
{
    send_time = millis() + 1000*30;
    xTaskCreate(send_state_task, "send_state_task", 4096, NULL, 10, NULL);
}