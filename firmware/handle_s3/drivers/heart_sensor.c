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

#define MAX30102_REG_INT_STATUS_1    0x00
#define MAX30102_REG_INT_STATUS_2    0x01
#define MAX30102_REG_FIFO_WR_PTR     0x04
#define MAX30102_REG_OVF_COUNTER     0x05
#define MAX30102_REG_FIFO_RD_PTR     0x06
#define MAX30102_REG_FIFO_DATA       0x07
#define MAX30102_REG_FIFO_CONFIG     0x08
#define MAX30102_REG_MODE_CONFIG     0x09
#define MAX30102_REG_SPO2_CONFIG     0x0A
#define MAX30102_REG_LED1_PA         0x0C
#define MAX30102_REG_LED2_PA         0x0D
#define MAX30102_REG_PART_ID         0xFF

static const uint8_t k_candidate_addrs[] = {
    0x57,
    0x58,
    0x48,
};

static void log_i2c_scan(void)
{
    bool any = false;
    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        if (i2c_bus_probe(addr) == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found addr=0x%02x", addr);
            any = true;
        }
    }
    if (!any) {
        ESP_LOGW(TAG, "I2C scan found no devices on SDA=GPIO%d SCL=GPIO%d",
                 HANDLE_I2C_SDA_GPIO, HANDLE_I2C_SCL_GPIO);
    }
}

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
    s_status.part_id = 0;
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
        i2c_bus_read_byte(s_status.i2c_addr, MAX30102_REG_PART_ID, &s_status.part_id);

        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_MODE_CONFIG, 0x40));
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_FIFO_WR_PTR, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_OVF_COUNTER, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_FIFO_RD_PTR, 0x00));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_FIFO_CONFIG, 0x4F));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_SPO2_CONFIG, 0x27));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_LED1_PA, 0x24));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_LED2_PA, 0x24));
        ESP_ERROR_CHECK(i2c_bus_write_byte(s_status.i2c_addr, MAX30102_REG_MODE_CONFIG, 0x03));

        uint8_t int_status_1 = 0;
        uint8_t int_status_2 = 0;
        i2c_bus_read_byte(s_status.i2c_addr, MAX30102_REG_INT_STATUS_1, &int_status_1);
        i2c_bus_read_byte(s_status.i2c_addr, MAX30102_REG_INT_STATUS_2, &int_status_2);
        ESP_LOGI(TAG, "MAX30102 connected: addr=0x%02x part=0x%02x INT=GPIO%d level=%d int=0x%02x/0x%02x",
                 s_status.i2c_addr,
                 s_status.part_id,
                 HANDLE_HEART_INT_GPIO,
                 s_status.int_level,
                 int_status_1,
                 int_status_2);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "not found on I2C: SDA=GPIO%d SCL=GPIO%d INT=GPIO%d",
             HANDLE_I2C_SDA_GPIO, HANDLE_I2C_SCL_GPIO, HANDLE_HEART_INT_GPIO);
    log_i2c_scan();
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

    uint8_t fifo[6] = {0};
    esp_err_t err = i2c_bus_read_bytes(s_status.i2c_addr, MAX30102_REG_FIFO_DATA, fifo, sizeof(fifo));
    if (err != ESP_OK) {
        out->valid = false;
        return err;
    }

    out->red = (((uint32_t)fifo[0] << 16) | ((uint32_t)fifo[1] << 8) | fifo[2]) & 0x03FFFF;
    out->ir = (((uint32_t)fifo[3] << 16) | ((uint32_t)fifo[4] << 8) | fifo[5]) & 0x03FFFF;
    out->valid = true;
    return ESP_OK;
}
