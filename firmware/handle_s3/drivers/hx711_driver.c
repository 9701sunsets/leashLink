#include "hx711_driver.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "board_pins.h"

static const char *TAG = "hx711";
/**
 * HX711校准参数
 */
static hx711_calibration_t s_cal = {
    .zero_offset = 0,
    .scale_counts_per_n = 12000.0f,
};

/**
 * 初始化HX711
 */
esp_err_t hx711_init(void)
{
    gpio_config_t dout_cfg = {
        .pin_bit_mask = 1ULL << HANDLE_HX711_DOUT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config_t sck_cfg = {
        .pin_bit_mask = 1ULL << HANDLE_HX711_SCK_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&dout_cfg));
    ESP_ERROR_CHECK(gpio_config(&sck_cfg));
    gpio_set_level(HANDLE_HX711_SCK_GPIO, 0);
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

/**
 * 读取原始ADC值
 * @param raw 输出原始ADC值
 */
esp_err_t hx711_read_raw(int32_t *raw)
{
    if (!raw) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t wait_us = 0;
    while (gpio_get_level(HANDLE_HX711_DOUT_GPIO) != 0) {
        esp_rom_delay_us(50);
        wait_us += 50;
        if (wait_us > 150000) {
            return ESP_ERR_TIMEOUT;
        }
    }

    int32_t value = 0;
    for (int i = 0; i < 24; ++i) {
        gpio_set_level(HANDLE_HX711_SCK_GPIO, 1);
        esp_rom_delay_us(1);
        value = (value << 1) | gpio_get_level(HANDLE_HX711_DOUT_GPIO);
        gpio_set_level(HANDLE_HX711_SCK_GPIO, 0);
        esp_rom_delay_us(1);
    }

    gpio_set_level(HANDLE_HX711_SCK_GPIO, 1);
    esp_rom_delay_us(1);
    gpio_set_level(HANDLE_HX711_SCK_GPIO, 0);

    if (value & 0x800000) {
        value |= 0xFF000000;
    }
    *raw = value;
    return ESP_OK;
}

/**
 * 设置校准参数
 * @param cal 校准参数
 */
esp_err_t hx711_set_calibration(hx711_calibration_t cal)
{
    if (cal.scale_counts_per_n <= 1.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    s_cal = cal;
    return ESP_OK;
}

/**
 * 读取张力值（牛顿）
 * @param newtons 输出张力值
 */
esp_err_t hx711_read_newtons(float *newtons)
{
    int32_t raw = 0;
    esp_err_t err = hx711_read_raw(&raw);
    if (err != ESP_OK) {
        return err;
    }
    *newtons = (float)(raw - s_cal.zero_offset) / s_cal.scale_counts_per_n;
    if (*newtons < 0.0f) {
        *newtons = 0.0f;
    }
    return ESP_OK;
}

/**
 * 执行去皮操作，计算零点偏移
 * @param samples 采样次数
 */
esp_err_t hx711_tare(uint16_t samples)
{
    if (samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t sum = 0;
    for (uint16_t i = 0; i < samples; ++i) {
        int32_t raw = 0;
        ESP_ERROR_CHECK(hx711_read_raw(&raw));
        sum += raw;
    }
    s_cal.zero_offset = (int32_t)(sum / samples);
    ESP_LOGI(TAG, "tare offset=%ld", (long)s_cal.zero_offset);
    return ESP_OK;
}
