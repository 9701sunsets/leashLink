#include "oled_driver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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

static void font5x7(char c, uint8_t out[5])
{
    memset(out, 0, 5);
    switch (c) {
    case '0': memcpy(out, (uint8_t[]){0x3E,0x51,0x49,0x45,0x3E}, 5); break;
    case '1': memcpy(out, (uint8_t[]){0x00,0x42,0x7F,0x40,0x00}, 5); break;
    case '2': memcpy(out, (uint8_t[]){0x42,0x61,0x51,0x49,0x46}, 5); break;
    case '3': memcpy(out, (uint8_t[]){0x21,0x41,0x45,0x4B,0x31}, 5); break;
    case '4': memcpy(out, (uint8_t[]){0x18,0x14,0x12,0x7F,0x10}, 5); break;
    case '5': memcpy(out, (uint8_t[]){0x27,0x45,0x45,0x45,0x39}, 5); break;
    case '6': memcpy(out, (uint8_t[]){0x3C,0x4A,0x49,0x49,0x30}, 5); break;
    case '7': memcpy(out, (uint8_t[]){0x01,0x71,0x09,0x05,0x03}, 5); break;
    case '8': memcpy(out, (uint8_t[]){0x36,0x49,0x49,0x49,0x36}, 5); break;
    case '9': memcpy(out, (uint8_t[]){0x06,0x49,0x49,0x29,0x1E}, 5); break;
    case 'A': memcpy(out, (uint8_t[]){0x7E,0x11,0x11,0x11,0x7E}, 5); break;
    case 'B': memcpy(out, (uint8_t[]){0x7F,0x49,0x49,0x49,0x36}, 5); break;
    case 'C': memcpy(out, (uint8_t[]){0x3E,0x41,0x41,0x41,0x22}, 5); break;
    case 'D': memcpy(out, (uint8_t[]){0x7F,0x41,0x41,0x22,0x1C}, 5); break;
    case 'E': memcpy(out, (uint8_t[]){0x7F,0x49,0x49,0x49,0x41}, 5); break;
    case 'F': memcpy(out, (uint8_t[]){0x7F,0x09,0x09,0x09,0x01}, 5); break;
    case 'G': memcpy(out, (uint8_t[]){0x3E,0x41,0x49,0x49,0x7A}, 5); break;
    case 'H': memcpy(out, (uint8_t[]){0x7F,0x08,0x08,0x08,0x7F}, 5); break;
    case 'I': memcpy(out, (uint8_t[]){0x00,0x41,0x7F,0x41,0x00}, 5); break;
    case 'K': memcpy(out, (uint8_t[]){0x7F,0x08,0x14,0x22,0x41}, 5); break;
    case 'L': memcpy(out, (uint8_t[]){0x7F,0x40,0x40,0x40,0x40}, 5); break;
    case 'M': memcpy(out, (uint8_t[]){0x7F,0x02,0x0C,0x02,0x7F}, 5); break;
    case 'N': memcpy(out, (uint8_t[]){0x7F,0x04,0x08,0x10,0x7F}, 5); break;
    case 'O': memcpy(out, (uint8_t[]){0x3E,0x41,0x41,0x41,0x3E}, 5); break;
    case 'P': memcpy(out, (uint8_t[]){0x7F,0x09,0x09,0x09,0x06}, 5); break;
    case 'R': memcpy(out, (uint8_t[]){0x7F,0x09,0x19,0x29,0x46}, 5); break;
    case 'S': memcpy(out, (uint8_t[]){0x46,0x49,0x49,0x49,0x31}, 5); break;
    case 'T': memcpy(out, (uint8_t[]){0x01,0x01,0x7F,0x01,0x01}, 5); break;
    case 'U': memcpy(out, (uint8_t[]){0x3F,0x40,0x40,0x40,0x3F}, 5); break;
    case 'V': memcpy(out, (uint8_t[]){0x1F,0x20,0x40,0x20,0x1F}, 5); break;
    case 'Y': memcpy(out, (uint8_t[]){0x07,0x08,0x70,0x08,0x07}, 5); break;
    case ':': memcpy(out, (uint8_t[]){0x00,0x36,0x36,0x00,0x00}, 5); break;
    case '-': memcpy(out, (uint8_t[]){0x08,0x08,0x08,0x08,0x08}, 5); break;
    case '/': memcpy(out, (uint8_t[]){0x20,0x10,0x08,0x04,0x02}, 5); break;
    case '.': memcpy(out, (uint8_t[]){0x00,0x60,0x60,0x00,0x00}, 5); break;
    case '%': memcpy(out, (uint8_t[]){0x23,0x13,0x08,0x64,0x62}, 5); break;
    default: break;
    }
}

static esp_err_t oled_draw_text(uint8_t page, const char *text)
{
    uint8_t pixels[128] = {0};
    uint8_t col = 0;
    for (const char *p = text; p && *p && col < 122; ++p) {
        uint8_t glyph[5];
        char c = *p;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        font5x7(c, glyph);
        memcpy(&pixels[col], glyph, 5);
        col += 6;
    }
    ESP_ERROR_CHECK(oled_set_page_col(page, 0));
    return oled_data(pixels, sizeof(pixels));
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
    char l0[22];
    char l1[22];
    char l2[22];
    char l3[22];
    snprintf(l0, sizeof(l0), "BAT:%d%% SAF:%d", battery_pct, safety_state);
    snprintf(l1, sizeof(l1), "TEN:%dN PK:%d", (int)(tension ? tension->tension_n : 0.0f),
             (int)(tension ? tension->tension_peak_n : 0.0f));
    snprintf(l2, sizeof(l2), "M:%u CB:%u",
             (unsigned)(collar ? collar->motion_state : LL_MOTION_UNKNOWN),
             (unsigned)(collar ? collar->battery_pct : 0));
    snprintf(l3, sizeof(l3), "RSSI:%d", collar ? collar->rssi_dbm : -100);
    return oled_show_lines(l0, l1, l2, l3);
}

/**
 * 显示消息
 * @param line1 第一行文本
 * @param line2 第二行文本
 */
esp_err_t oled_show_message(const char *line1, const char *line2)
{
    ESP_LOGI(TAG, "%s | %s", line1 ? line1 : "", line2 ? line2 : "");
    return oled_show_lines(line1, line2, NULL, NULL);
}

esp_err_t oled_show_lines(const char *line0,
                          const char *line1,
                          const char *line2,
                          const char *line3)
{
    if (!s_ready) {
        return ESP_OK;
    }
    ESP_ERROR_CHECK(oled_fill(0x00));
    ESP_ERROR_CHECK(oled_draw_text(0, line0 ? line0 : ""));
    ESP_ERROR_CHECK(oled_draw_text(2, line1 ? line1 : ""));
    ESP_ERROR_CHECK(oled_draw_text(4, line2 ? line2 : ""));
    ESP_ERROR_CHECK(oled_draw_text(6, line3 ? line3 : ""));
    return ESP_OK;
}
