#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_pairing.h"
#include "buzzer_driver.h"
#include "cloud_service.h"
#include "distance_service.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "leash_control.h"
#include "motor_driver.h"
#include "oled_driver.h"
#include "power.h"
#include "safety_service.h"
#include "session_service.h"
#include "tension_service.h"

void task_tension(void *arg);
void task_safety(void *arg);
void task_actuator(void *arg);
void task_ui(void *arg);
void task_cloud(void *arg);

static const char *TAG = "app_main";

/**
 * GPS轮询任务
 */
static void task_gps(void *arg)
{
    (void)arg;
    while (true) {
        gps_poll(NULL);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink handle_c3 boot");

    ESP_ERROR_CHECK(power_init());// 初始化电源管理
    ESP_ERROR_CHECK(oled_init());// 初始化OLED显示屏
    ESP_ERROR_CHECK(buzzer_init());// 初始化蜂鸣器
    ESP_ERROR_CHECK(motor_init());// 初始化电机驱动
    ESP_ERROR_CHECK(leash_control_init());// 初始化牵引控制
    ESP_ERROR_CHECK(tension_service_init());// 初始化张力服务
    ESP_ERROR_CHECK(gps_init());// 初始化GPS驱动
    ESP_ERROR_CHECK(distance_service_init());// 初始化距离服务
    ESP_ERROR_CHECK(safety_service_init(NULL));// 初始化安全服务
    ESP_ERROR_CHECK(session_service_init());// 初始化会话服务
    ESP_ERROR_CHECK(espnow_handle_init());// 初始化ESP-NOW通信处理
    ESP_ERROR_CHECK(ble_pairing_init());// 初始化BLE配对服务
    ESP_ERROR_CHECK(cloud_service_init());// 初始化云服务

    xTaskCreate(task_tension, "task_tension", 4096, NULL, 8, NULL);// 创建张力采样任务
    xTaskCreate(task_safety, "task_safety", 4096, NULL, 9, NULL);// 创建安全评估任务
    xTaskCreate(task_actuator, "task_actuator", 3072, NULL, 7, NULL);// 创建执行器任务
    xTaskCreate(task_ui, "task_ui", 4096, NULL, 4, NULL);// 创建UI任务
    xTaskCreate(task_cloud, "task_cloud", 4096, NULL, 3, NULL);// 创建云端任务
    xTaskCreate(task_gps, "task_gps", 4096, NULL, 4, NULL);// 创建GPS轮询任务

    oled_show_message("leashLink", "handle ready");
}

