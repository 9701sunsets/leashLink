#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"

// I2C引脚
#define HANDLE_I2C_SCL_GPIO       GPIO_NUM_4
#define HANDLE_I2C_SDA_GPIO       GPIO_NUM_5
#define HANDLE_I2C_PORT_NUM       I2C_NUM_0

// GPS引脚
#define HANDLE_GPS_TX_GPIO        GPIO_NUM_17
#define HANDLE_GPS_RX_GPIO        GPIO_NUM_18
#define HANDLE_GPS_PPS_GPIO       GPIO_NUM_16
#define HANDLE_GPS_UART_NUM       UART_NUM_1

// HX711引脚
#define HANDLE_HX711_DOUT_GPIO    GPIO_NUM_6
#define HANDLE_HX711_SCK_GPIO     GPIO_NUM_7

// LED引脚
#define HANDLE_LIGHT_AO_GPIO      GPIO_NUM_1
#define HANDLE_LIGHT_DO_GPIO      GPIO_NUM_2
#define HANDLE_HEART_INT_GPIO     GPIO_NUM_9

#define HANDLE_SERVO_PWM_GPIO     GPIO_NUM_10
#define HANDLE_MOTOR_GPIO         GPIO_NUM_11
#define HANDLE_BUZZER_GPIO        GPIO_NUM_12

// ADC通道
#define HANDLE_LIGHT_ADC_CHANNEL  ADC_CHANNEL_0
#define HANDLE_BAT_ADC_GPIO       GPIO_NUM_3
#define HANDLE_BAT_ADC_CHANNEL    ADC_CHANNEL_2

// 按键引脚
#define HANDLE_BUTTON_1_GPIO      GPIO_NUM_13
#define HANDLE_BUTTON_2_GPIO      GPIO_NUM_14
#define HANDLE_BUTTON_3_GPIO      GPIO_NUM_15

// LEDC配置
#define HANDLE_SERVO_LEDC_TIMER   LEDC_TIMER_0
#define HANDLE_SERVO_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define HANDLE_SERVO_LEDC_CH      LEDC_CHANNEL_0
