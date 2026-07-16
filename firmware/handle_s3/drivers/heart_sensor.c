#include "heart_sensor.h"

#include <stddef.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_bus.h"

static const char *TAG = "heart_sensor";
static ll_heart_sensor_status_t s_status;

#define MAX30100_REG_INT_STATUS      0x00
#define MAX30100_REG_FIFO_WR_PTR     0x02
#define MAX30100_REG_OVF_COUNTER     0x03
#define MAX30100_REG_FIFO_RD_PTR     0x04
#define MAX30100_REG_FIFO_DATA       0x05
#define MAX30100_REG_MODE_CONFIG     0x06
#define MAX30100_REG_SPO2_CONFIG     0x07
#define MAX30100_REG_LED_CONFIG      0x09

static const uint8_t k_candidate_addrs[] = {
    0x57,
    0x58,
    0x48,
};

esp_err_t heart_sensor_init(void)
{
    gpio_config_t int_cfg = {
        .pin_bit_mask = 1ULL << HANDLE_HEART_INT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&int_cfg));

    ESP_ERROR_CHECK(i2c_bus_init());

    s_status.present = false;
    s_status.i2c_addr = 0;
    for (size_t i = 0; i < sizeof(k_candidate_addrs); ++i) {
        uint8_t addr = k_candidate_addrs[i];
        if (i2c_bus_probe(addr) == ESP_OK) {
            s_status.present = true;
            s_status.i2c_addr = addr;
            break;
        }
    }

    s_status.int_level = gpio_get_level(HANDLE_HEART_INT_GPIO);
    if (s_status.present) {
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_MODE_CONFIG, 0x40));
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_FIFO_WR_PTR, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_OVF_COUNTER, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_FIFO_RD_PTR, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_SPO2_CONFIG, 0x27));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_LED_CONFIG, 0x24));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30100_REG_MODE_CONFIG, 0x03));

        uint8_t int_status = 0;
        i2c_bus_read_byte(s_status.i2c_addr, MAX30100_REG_INT_STATUS, &int_status);
        ESP_LOGI(TAG, "MAX30100/MAX3010x connected: addr=0x%02x INT=GPIO%d level=%d int_status=0x%02x",
                 s_status.i2c_addr, HANDLE_HEART_INT_GPIO, s_status.int_level, int_status);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "not found on I2C: SDA=GPIO%d SCL=GPIO%d INT=GPIO%d",
             HANDLE_I2C_SDA_GPIO, HANDLE_I2C_SCL_GPIO, HANDLE_HEART_INT_GPIO);
    return ESP_ERR_NOT_FOUND;
}

ll_heart_sensor_status_t heart_sensor_get_status(void)
{
    s_status.int_level = gpio_get_level(HANDLE_HEART_INT_GPIO);
    return s_status;
}

esp_err_t heart_sensor_read_raw(ll_heart_raw_sample_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_status.present) {
        out->valid = false;
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t fifo[4] = {0};
    esp_err_t err = i2c_bus_read_bytes(s_status.i2c_addr, MAX30100_REG_FIFO_DATA, fifo, sizeof(fifo));
    if (err != ESP_OK) {
        out->valid = false;
        return err;
    }

    out->ir = ((uint16_t)fifo[0] << 8) | fifo[1];
    out->red = ((uint16_t)fifo[2] << 8) | fifo[3];
    out->valid = true;
    return ESP_OK;
}
