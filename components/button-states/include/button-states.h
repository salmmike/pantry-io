/* Button (or GPIO) control functions */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define BOUNCE_TIME_MS 50
#define GPIO_INPUT_IO_0      17
#define GPIO_INPUT_IO_1      5
#define GPIO_OUTPUT_1        18
#define ESP_INTR_FLAG_DEFAULT 0

typedef struct {
  uint8_t pin;
  bool state;
  uint32_t previous_time;
} button_t;

void init_gpio();
void init_state_sender();
char* get_button_state_json();
