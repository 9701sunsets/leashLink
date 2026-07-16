#include "oled_driver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

static const char *TAG = "oled";
static uint8_t s_oled_addr;
static bool s_ready;

static esp_err_t oled_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    return i2c_bus_write(s_oled_addr, data, sizeof(data));
}

static esp_err_t oled_data(const uint8_t *data, size_t len)
{
    uint8_t buf[17] = {0x40};
    while (len > 0) {
        size_t chunk = len > 16 ? 16 : len;
        memcpy(&buf[1], data, chunk);
        ESP_ERROR_CHECK(i2c_bus_write(s_oled_addr, buf, chunk + 1));
        data += chunk;
        len -= chunk;
    }
    return ESP_OK;
}

static esp_err_t oled_set_page_col(uint8_t page, uint8_t col)
{
    ESP_ERROR_CHECK(oled_cmd(0xB0 | (page & 0x07)));
    ESP_ERROR_CHECK(oled_cmd(0x00 | (col & 0x0F)));
    ESP_ERROR_CHECK(oled_cmd(0x10 | ((col >> 4) & 0x0F)));
    return ESP_OK;
}

static esp_err_t oled_fill(uint8_t pattern)
{
    uint8_t line[128];
    memset(line, pattern, sizeof(line));
    for (uint8_t page = 0; page < 8; ++page) {
        ESP_ERROR_CHECK(oled_set_page_col(page, 0));
        ESP_ERROR_CHECK(oled_data(line, sizeof(line)));
    }
    return ESP_OK;
}

/**
 * 初始化OLED显示屏
 */
esp_err_t oled_init(void)
{
    ESP_ERROR_CHECK(i2c_bus_init());

    if (i2c_bus_probe(0x3C) == ESP_OK) {
        s_oled_addr = 0x3C;
    } else if (i2c_bus_probe(0x3D) == ESP_OK) {
        s_oled_addr = 0x3D;
    } else {
        ESP_LOGW(TAG, "OLED not found on I2C address 0x3C/0x3D");
        return ESP_ERR_NOT_FOUND;
    }

    const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0x2E, 0xAF,
    };
    for (size_t i = 0; i < sizeof(init_cmds); ++i) {
        ESP_ERROR_CHECK(oled_cmd(init_cmds[i]));
    }

    s_ready = true;
    ESP_ERROR_CHECK(oled_fill(0xFF));
    vTaskDelay(pdMS_TO_TICKS(250));
    ESP_ERROR_CHECK(oled_fill(0x00));

    ESP_LOGI(TAG, "connected: SSD1306-compatible OLED addr=0x%02x", s_oled_addr);
    return ESP_OK;
}

/**
 * 显示状态信息
 * @param tension 张力样本
 * @param collar 项圈遥测数据
 * @param safety_state 安全状态
 * @param battery_pct 电池百分比
 */
esp_err_t oled_show_status(const ll_tension_sample_t *tension,
                           const ll_collar_telemetry_t *collar,
                           ll_safety_state_t safety_state,
                           int battery_pct)
{
    ESP_LOGI(TAG, "bat=%d%% tension=%.1fN peak=%.1fN motion=%d safety=%d",
             battery_pct,
             tension ? tension->tension_n : 0.0f,
             tension ? tension->tension_peak_n : 0.0f,
             collar ? collar->motion_state : LL_MOTION_UNKNOWN,
             safety_state);
    return ESP_OK;
}

/**
 * 显示消息
 * @param line1 第一行文本
 * @param line2 第二行文本
 */
esp_err_t oled_show_message(const char *line1, const char *line2)
{
    ESP_LOGI(TAG, "%s | %s", line1 ? line1 : "", line2 ? line2 : "");
    if (!s_ready) {
        return ESP_OK;
    }

    uint8_t line[128] = {0};
    memset(line, line1 && line1[0] ? 0x18 : 0x00, sizeof(line));
    ESP_ERROR_CHECK(oled_set_page_col(1, 0));
    ESP_ERROR_CHECK(oled_data(line, sizeof(line)));

    memset(line, line2 && line2[0] ? 0x3C : 0x00, sizeof(line));
    ESP_ERROR_CHECK(oled_set_page_col(3, 0));
    ESP_ERROR_CHECK(oled_data(line, sizeof(line)));
    return ESP_OK;
}
