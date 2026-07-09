#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define COLLAR_I2C_SCL_GPIO       GPIO_NUM_18
#define COLLAR_I2C_SDA_GPIO       GPIO_NUM_19
#define COLLAR_LED_DATA_GPIO      GPIO_NUM_8
#define COLLAR_BUZZER_GPIO        GPIO_NUM_6
#define COLLAR_MOTOR_GPIO         GPIO_NUM_7
#define COLLAR_BAT_ADC_CHANNEL    ADC_CHANNEL_0
#define COLLAR_KEY_GPIO           GPIO_NUM_9
