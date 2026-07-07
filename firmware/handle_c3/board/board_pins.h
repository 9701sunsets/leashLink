#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"

#define HANDLE_I2C_SCL_GPIO       GPIO_NUM_4
#define HANDLE_I2C_SDA_GPIO       GPIO_NUM_5

#define HANDLE_GPS_RX_GPIO        GPIO_NUM_20
#define HANDLE_GPS_TX_GPIO        GPIO_NUM_21
#define HANDLE_GPS_UART_NUM       UART_NUM_1

#define HANDLE_HX711_DOUT_GPIO    GPIO_NUM_6
#define HANDLE_HX711_SCK_GPIO     GPIO_NUM_7

#define HANDLE_SERVO_PWM_GPIO     GPIO_NUM_2
#define HANDLE_BUZZER_GPIO        GPIO_NUM_3
#define HANDLE_MOTOR_GPIO         GPIO_NUM_10

#define HANDLE_LIGHT_ADC_CHANNEL  ADC_CHANNEL_0
#define HANDLE_BAT_ADC_CHANNEL    ADC_CHANNEL_1

#define HANDLE_PAIR_KEY_GPIO      GPIO_NUM_8
#define HANDLE_FIND_KEY_GPIO      GPIO_NUM_9

#define HANDLE_SERVO_LEDC_TIMER   LEDC_TIMER_0
#define HANDLE_SERVO_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define HANDLE_SERVO_LEDC_CH      LEDC_CHANNEL_0
