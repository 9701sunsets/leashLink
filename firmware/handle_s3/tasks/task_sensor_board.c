#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "button_driver.h"
#include "esp_log.h"
#include "heart_sensor.h"
#include "light_sensor.h"
#include "oled_driver.h"

static const char *TAG = "task_sensor_board";

void task_sensor_board(void *arg)
{
    (void)arg;

    while (true) {
        ll_light_sample_t light = {0};
        ll_heart_sensor_status_t heart = heart_sensor_get_status();
        ll_heart_raw_sample_t heart_raw = {0};
        ll_button_state_t buttons = button_driver_get_state();
        heart_sensor_read_raw(&heart_raw);

        if (light_sensor_read(&light) == ESP_OK) {
            ESP_LOGI(TAG, "light raw=%d lux_est=%d DO_dark=%d heart_present=%d addr=0x%02x part=0x%02x INT=%d ir=%lu red=%lu buttons=%d/%d/%d",
                     light.raw,
                     light.lux_est,
                     light.digital_dark,
                     heart.present,
                     heart.i2c_addr,
                     heart.part_id,
                     heart.int_level,
                     (unsigned long)heart_raw.ir,
                     (unsigned long)heart_raw.red,
                     buttons.button1_pressed,
                     buttons.button2_pressed,
                     buttons.button3_pressed);
        }

        oled_show_message("sensor board", heart.present ? "heart ok" : "heart missing");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
