#include "imu_driver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "i2c_bus.h"

static const char *TAG = "imu";
static collar_imu_status_t s_status;

#define BMI270_CHIP_ID_REG 0x00
#define BMM350_CHIP_ID_REG 0x00

static const uint8_t k_bmi270_addrs[] = {0x68, 0x69};
static const uint8_t k_bmm350_addrs[] = {0x14, 0x15};

static void imu_log_i2c_scan(void)
{
    bool any = false;
    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        if (collar_i2c_probe(addr) == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found addr=0x%02x", addr);
            any = true;
        }
    }
    if (!any) {
        ESP_LOGW(TAG, "I2C scan found no devices on SDA=GPIO%d SCL=GPIO%d",
                 COLLAR_I2C_SDA_GPIO, COLLAR_I2C_SCL_GPIO);
    }
}

/**
 * 初始化IMU传感器
 */
esp_err_t imu_init(void)
{
    gpio_config_t int_cfg = {
        .pin_bit_mask = (1ULL << COLLAR_BMI_INT1_GPIO) |
                        (1ULL << COLLAR_BMI_INT2_GPIO) |
                        (1ULL << COLLAR_BMI_CS_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&int_cfg));
    gpio_set_level(COLLAR_BMI_CS_GPIO, 1);

    ESP_ERROR_CHECK(collar_i2c_bus_init());

    for (size_t i = 0; i < sizeof(k_bmi270_addrs); ++i) {
        uint8_t addr = k_bmi270_addrs[i];
        if (collar_i2c_probe(addr) == ESP_OK &&
            collar_i2c_read_byte(addr, BMI270_CHIP_ID_REG, &s_status.bmi270_chip_id) == ESP_OK) {
            s_status.bmi270_present = true;
            s_status.bmi270_addr = addr;
            break;
        }
    }

    for (size_t i = 0; i < sizeof(k_bmm350_addrs); ++i) {
        uint8_t addr = k_bmm350_addrs[i];
        if (collar_i2c_probe(addr) == ESP_OK &&
            collar_i2c_read_byte(addr, BMM350_CHIP_ID_REG, &s_status.bmm350_chip_id) == ESP_OK) {
            s_status.bmm350_present = true;
            s_status.bmm350_addr = addr;
            break;
        }
    }

    ESP_LOGI(TAG, "BMI270 present=%d addr=0x%02x chip=0x%02x INT1=GPIO%d INT2=GPIO%d",
             s_status.bmi270_present,
             s_status.bmi270_addr,
             s_status.bmi270_chip_id,
             COLLAR_BMI_INT1_GPIO,
             COLLAR_BMI_INT2_GPIO);
    ESP_LOGI(TAG, "BMM350 present=%d addr=0x%02x chip=0x%02x",
             s_status.bmm350_present,
             s_status.bmm350_addr,
             s_status.bmm350_chip_id);
    if (!s_status.bmi270_present && !s_status.bmm350_present) {
        imu_log_i2c_scan();
    }
    return ESP_OK;
}

/**
 * 读取加速度峰值
 * @param peak_mg 加速度峰值，单位为mg
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t imu_read_accel_peak(uint16_t *peak_mg)
{
    if (!peak_mg) {
        return ESP_ERR_INVALID_ARG;
    }
    *peak_mg = 0;
    return ESP_OK;
}

/**
 * 读取温度
 * @param temp_c_x10 温度值，单位为摄氏度*10
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t imu_read_temp(int16_t *temp_c_x10)
{
    if (!temp_c_x10) {
        return ESP_ERR_INVALID_ARG;
    }
    *temp_c_x10 = 0;
    return ESP_OK;
}

collar_imu_status_t imu_get_status(void)
{
    return s_status;
}
