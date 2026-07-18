#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool button1_pressed;
    bool button2_pressed;
    bool button3_pressed;
    int button1_level;
    int button2_level;
    int button3_level;
} ll_button_state_t;

esp_err_t button_driver_init(void);
ll_button_state_t button_driver_get_state(void);
bool button_driver_is_pressed(uint8_t index);
