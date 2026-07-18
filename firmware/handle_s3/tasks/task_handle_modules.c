#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "button_driver.h"
#include "buzzer_driver.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "heart_sensor.h"
#include "leash_control.h"
#include "light_sensor.h"
#include "oled_driver.h"
#include "power.h"

static const char *TAG = "task_handle_modules";

void task_handle_modules(void *arg)
{
    (void)arg;
    ll_button_state_t prev_buttons = {0};
    while (true) {
        ll_light_sample_t light = {0};
        esp_err_t light_err = light_sensor_read(&light);
        ll_heart_sensor_status_t heart_status = heart_sensor_get_status();
        ll_heart_raw_sample_t heart_raw = {0};
        esp_err_t heart_err = heart_sensor_read_raw(&heart_raw);
        ll_gps_fix_t gps = gps_get_latest();
        ll_collar_telemetry_t collar = espnow_handle_get_collar();
        ll_button_state_t buttons = button_driver_get_state();
        int battery_pct = power_get_battery_pct();
        bool link_ok = espnow_handle_link_ok();
        ll_leash_state_t leash = leash_control_get_state();

        if (buttons.button1_pressed && !prev_buttons.button1_pressed) {
            ESP_LOGI(TAG, "manual test B1: servo unlock");
            leash_control_unlock();
            buzzer_beep(40);
        }
        if (buttons.button2_pressed && !prev_buttons.button2_pressed) {
            ESP_LOGI(TAG, "manual test B2: servo lock");
            leash_control_lock(0);
            buzzer_beep(120);
        }
        if (buttons.button3_pressed && !prev_buttons.button3_pressed) {
            ESP_LOGI(TAG, "manual test B3: send collar feedback warning");
            espnow_handle_send_feedback(2, 300);
            buzzer_beep(60);
        }
        prev_buttons = buttons;
        leash = leash_control_get_state();

        ESP_LOGI(TAG,
                 "handle data bat=%d light_raw=%d lux=%d dark=%d light_ok=%d "
                 "heart_present=%d addr=0x%02x INT=%d heart_ok=%d ir=%u red=%u "
                 "gps_fix=%d lat=%.6f lng=%.6f acc=%.1f "
                 "link=%d collar_rssi=%d collar_motion=%u collar_bat=%u "
                 "leash=%u buttons=%d/%d/%d",
                 battery_pct,
                 light.raw,
                 light.lux_est,
                 light.digital_dark,
                 light_err == ESP_OK,
                 heart_status.present,
                 heart_status.i2c_addr,
                 heart_status.int_level,
                 heart_err == ESP_OK && heart_raw.valid,
                 heart_raw.ir,
                 heart_raw.red,
                 gps.fix,
                 gps.lat,
                 gps.lng,
                 gps.accuracy_m,
                 link_ok,
                 collar.rssi_dbm,
                 (unsigned)collar.motion_state,
                 (unsigned)collar.battery_pct,
                 (unsigned)leash,
                 buttons.button1_pressed,
                 buttons.button2_pressed,
                 buttons.button3_pressed);

        char l0[22];
        char l1[22];
        char l2[22];
        char l3[22];
        snprintf(l0, sizeof(l0), "BAT:%d%% C:%d", battery_pct, collar.rssi_dbm);
        snprintf(l1, sizeof(l1), "L:%d D:%d M:%u",
                 light.lux_est, light.digital_dark, (unsigned)collar.motion_state);
        snprintf(l2, sizeof(l2), "HR:%u/%u", heart_raw.ir, heart_raw.red);
        snprintf(l3, sizeof(l3), "GPS:%c CB:%u", gps.fix ? 'Y' : 'N', (unsigned)collar.battery_pct);
        oled_show_lines(l0, l1, l2, l3);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
