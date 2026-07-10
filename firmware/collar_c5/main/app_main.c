#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_adv.h"
#include "buzzer_driver.h"
#include "espnow_handle.h"
#include "feedback_service.h"
#include "imu_driver.h"
#include "led_driver.h"
#include "motion_service.h"
#include "power_service.h"
#include "vibration_driver.h"

static const char *TAG = "collar_main";

void task_imu(void *arg);
void task_feedback(void *arg);

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink collar C5 boot");

    ESP_ERROR_CHECK(power_service_init());// 初始化电源管理
    ESP_ERROR_CHECK(imu_init());// 初始化IMU传感器
    ESP_ERROR_CHECK(led_init());// 初始化LED驱动
    ESP_ERROR_CHECK(buzzer_init());// 初始化蜂鸣器驱动
    ESP_ERROR_CHECK(vibration_init());// 初始化振动电机驱动
    ESP_ERROR_CHECK(motion_service_init());// 初始化运动服务
    ESP_ERROR_CHECK(feedback_service_init());// 初始化反馈服务
    ESP_ERROR_CHECK(espnow_handle_init());// 初始化ESP-NOW通信
    ESP_ERROR_CHECK(ble_adv_init());// 初始化BLE广播

    xTaskCreatePinnedToCore(task_imu, "task_imu", 4096, NULL, 5, NULL, 0);// 创建IMU任务
    xTaskCreatePinnedToCore(task_feedback, "task_feedback", 4096, NULL, 4, NULL, 0);// 创建反馈任务

    ESP_LOGI(TAG, "collar tasks started");
}
