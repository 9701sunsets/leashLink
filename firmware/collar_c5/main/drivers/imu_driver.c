#include "imu_driver.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"
#include "bmi270_espp_bridge.h"
#include "i2c_bus.h"

static const char *TAG = "imu";
static collar_imu_status_t s_status;
static spi_device_handle_t s_bmi_spi;
static bool s_spi_bus_ready;

#define BMI270_CHIP_ID_REG 0x00
#define BMI270_ACC_X_LSB_REG 0x0C
#define BMI270_STATUS_REG 0x03
#define BMI270_INTERNAL_STATUS_REG 0x21
#define BMI270_ACC_CONF_REG 0x40
#define BMI270_ACC_RANGE_REG 0x41
#define BMI270_INIT_CTRL_REG 0x59
#define BMI270_INIT_ADDR_0_REG 0x5B
#define BMI270_INIT_ADDR_1_REG 0x5C
#define BMI270_INIT_DATA_REG 0x5E
#define BMI270_PWR_CONF_REG 0x7C
#define BMI270_PWR_CTRL_REG 0x7D
#define BMM350_CHIP_ID_REG 0x00

static const uint8_t k_bmi270_addrs[] = {0x68, 0x69};
static const uint8_t k_bmm350_addrs[] = {0x14, 0x15};

static esp_err_t bmi270_spi_init(void)
{
    if (!s_spi_bus_ready) {
        spi_bus_config_t buscfg = {
            .mosi_io_num = COLLAR_SHUTTLE_I2C_SDA,
            .miso_io_num = COLLAR_BM_SDO_GPIO,
            .sclk_io_num = COLLAR_SHUTTLE_I2C_SCL,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 64,
        };
        esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            return err;
        }
        s_spi_bus_ready = true;
    }

    if (!s_bmi_spi) {
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = 1000 * 1000,
            .mode = 0,
            .spics_io_num = COLLAR_BM_CS_GPIO,
            .queue_size = 1,
        };
        ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &devcfg, &s_bmi_spi), TAG, "add BMI270 SPI device failed");
    }
    return ESP_OK;
}

static esp_err_t bmi270_spi_read(uint8_t reg, uint8_t *data, size_t len)
{
    if (!data || len == 0 || len > 15) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(bmi270_spi_init(), TAG, "spi init failed");

    uint8_t tx[17] = {reg | 0x80, 0x00};
    uint8_t rx[17] = {0};
    spi_transaction_t trans = {
        .length = (len + 2) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    ESP_RETURN_ON_ERROR(spi_device_transmit(s_bmi_spi, &trans), TAG, "spi read failed");
    memcpy(data, &rx[2], len);
    return ESP_OK;
}

static esp_err_t bmi270_spi_write(uint8_t reg, uint8_t value)
{
    ESP_RETURN_ON_ERROR(bmi270_spi_init(), TAG, "spi init failed");
    uint8_t tx[2] = {reg & 0x7F, value};
    spi_transaction_t trans = {
        .length = sizeof(tx) * 8,
        .tx_buffer = tx,
    };
    return spi_device_transmit(s_bmi_spi, &trans);
}

static bool imu_probe_known_devices(void)
{
    s_status.bmi270_present = false;
    s_status.bmi270_spi = false;
    s_status.bmm350_present = false;
    s_status.bmi270_addr = 0;
    s_status.bmm350_addr = 0;
    s_status.bmi270_chip_id = 0;
    s_status.bmm350_chip_id = 0;
    s_status.config_loaded = false;

    if (collar_bmi270_espp_init() == ESP_OK) {
        s_status.bmi270_present = true;
        s_status.bmi270_spi = true;
        s_status.bmi270_chip_id = 0x24;
        s_status.config_loaded = collar_bmi270_espp_is_ready();
        return true;
    }

    uint8_t chip = 0;
    if (bmi270_spi_read(BMI270_CHIP_ID_REG, &chip, 1) == ESP_OK && chip == 0x24) {
        s_status.bmi270_present = true;
        s_status.bmi270_spi = true;
        s_status.bmi270_chip_id = chip;
        return true;
    }

    if (collar_i2c_bus_init() != ESP_OK) {
        ESP_LOGW(TAG, "I2C init failed after SPI probe");
        return false;
    }
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

    return s_status.bmi270_present || s_status.bmm350_present;
}

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
        ESP_LOGW(TAG, "I2C scan found no devices on SDA=GPIO%d SCL=GPIO%d G1=%d G2=%d SDO=%d CS=1",
                 COLLAR_I2C_SDA_GPIO,
                 COLLAR_I2C_SCL_GPIO,
                 s_status.shuttle_g1_level,
                 s_status.shuttle_g2_level,
                 s_status.shuttle_sdo_level);
    }
}

static void imu_set_shuttle_mode(uint8_t g1, uint8_t g2, uint8_t sdo)
{
    gpio_set_level(COLLAR_BM_CS_GPIO, 1);
    gpio_set_level(COLLAR_BM_G1_GPIO, g1);
    gpio_set_level(COLLAR_BM_G2_GPIO, g2);
    gpio_set_level(COLLAR_BM_SDO_GPIO, sdo);
    s_status.shuttle_g1_level = g1;
    s_status.shuttle_g2_level = g2;
    s_status.shuttle_sdo_level = sdo;
    vTaskDelay(pdMS_TO_TICKS(20));
}

static bool imu_find_working_shuttle_mode(void)
{
    for (uint8_t sdo = 0; sdo <= 1; ++sdo) {
        for (uint8_t g1 = 0; g1 <= 1; ++g1) {
            for (uint8_t g2 = 0; g2 <= 1; ++g2) {
                imu_set_shuttle_mode(g1, g2, sdo);
                ESP_LOGI(TAG, "probe shuttle mode G1=%u G2=%u SDO=%u CS=1",
                         g1, g2, sdo);
                if (imu_probe_known_devices()) {
                    ESP_LOGI(TAG, "selected shuttle mode G1=%u G2=%u SDO=%u",
                             g1, g2, sdo);
                    return true;
                }
            }
        }
    }
    return false;
}

static uint32_t isqrt_u32(uint32_t value)
{
    uint32_t root = 0;
    uint32_t bit = 1UL << 30;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static void bmi270_configure_basic_accel(void)
{
    if (!s_status.bmi270_present) {
        return;
    }
    if (s_status.bmi270_spi) {
        if (!s_status.config_loaded) {
            esp_err_t init_err = collar_bmi270_espp_init();
            s_status.config_loaded = init_err == ESP_OK && collar_bmi270_espp_is_ready();
            if (!s_status.config_loaded) {
                ESP_LOGW(TAG, "BMI270 espp init failed: %s", esp_err_to_name(init_err));
            }
        }
        ESP_LOGI(TAG, "BMI270 espp init cfg=%d chip=0x%02x range=4g odr=100hz G1=GPIO%d:%u G2=GPIO%d:%u CS=GPIO%d SDO=GPIO%d:%u",
                 s_status.config_loaded,
                 s_status.bmi270_chip_id,
                 COLLAR_BM_G1_GPIO,
                 s_status.shuttle_g1_level,
                 COLLAR_BM_G2_GPIO,
                 s_status.shuttle_g2_level,
                 COLLAR_BM_CS_GPIO,
                 COLLAR_BM_SDO_GPIO,
                 s_status.shuttle_sdo_level);
        return;
    }

    collar_i2c_write_byte(s_status.bmi270_addr, BMI270_PWR_CONF_REG, 0x00);
    vTaskDelay(pdMS_TO_TICKS(5));
    collar_i2c_write_byte(s_status.bmi270_addr, BMI270_ACC_CONF_REG, 0xA8);
    collar_i2c_write_byte(s_status.bmi270_addr, BMI270_ACC_RANGE_REG, 0x00);
    collar_i2c_write_byte(s_status.bmi270_addr, BMI270_PWR_CTRL_REG, 0x04);
    vTaskDelay(pdMS_TO_TICKS(20));
    collar_i2c_read_byte(s_status.bmi270_addr, BMI270_STATUS_REG, &s_status.status_reg);
    collar_i2c_read_byte(s_status.bmi270_addr, BMI270_PWR_CTRL_REG, &s_status.pwr_ctrl_reg);
    collar_i2c_read_byte(s_status.bmi270_addr, BMI270_ACC_CONF_REG, &s_status.acc_conf_reg);
    collar_i2c_read_byte(s_status.bmi270_addr, BMI270_ACC_RANGE_REG, &s_status.acc_range_reg);
    ESP_LOGI(TAG, "BMI270 basic accel configured addr=0x%02x status=0x%02x pwr=0x%02x conf=0x%02x range=0x%02x",
             s_status.bmi270_addr,
             s_status.status_reg,
             s_status.pwr_ctrl_reg,
             s_status.acc_conf_reg,
             s_status.acc_range_reg);
}

/**
 * 初始化IMU传感器
 */
esp_err_t imu_init(void)
{
    gpio_config_t shuttle_cfg = {
        .pin_bit_mask = (1ULL << COLLAR_BM_G1_GPIO) |
                        (1ULL << COLLAR_BM_G2_GPIO) |
                        (1ULL << COLLAR_BM_CS_GPIO) |
                        (1ULL << COLLAR_BM_SDO_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&shuttle_cfg));
    imu_set_shuttle_mode(0, 0, 0);

    if (!imu_probe_known_devices()) {
        imu_find_working_shuttle_mode();
    }

    ESP_LOGI(TAG, "BMI270 present=%d bus=%s addr=0x%02x chip=0x%02x G1=GPIO%d:%u G2=GPIO%d:%u CS=GPIO%d:1 SDO=GPIO%d:%u",
             s_status.bmi270_present,
             s_status.bmi270_spi ? "spi" : "i2c",
             s_status.bmi270_addr,
             s_status.bmi270_chip_id,
             COLLAR_BM_G1_GPIO,
             s_status.shuttle_g1_level,
             COLLAR_BM_G2_GPIO,
             s_status.shuttle_g2_level,
             COLLAR_BM_CS_GPIO,
             COLLAR_BM_SDO_GPIO,
             s_status.shuttle_sdo_level);
    ESP_LOGI(TAG, "BMM350 present=%d addr=0x%02x chip=0x%02x",
             s_status.bmm350_present,
             s_status.bmm350_addr,
             s_status.bmm350_chip_id);
    if (!s_status.bmi270_present && !s_status.bmm350_present) {
        imu_log_i2c_scan();
    }
    bmi270_configure_basic_accel();
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
    if (!s_status.bmi270_present) {
        *peak_mg = 0;
        return ESP_OK;
    }
    if (!s_status.config_loaded) {
        memset(s_status.last_acc_raw, 0, sizeof(s_status.last_acc_raw));
        s_status.accel_x_mg = 0;
        s_status.accel_y_mg = 0;
        s_status.accel_z_mg = 0;
        *peak_mg = 0;
        return ESP_OK;
    }

    if (s_status.bmi270_spi && collar_bmi270_espp_is_ready()) {
        collar_bmi270_accel_t accel = {0};
        esp_err_t read_err = collar_bmi270_espp_read_accel(&accel);
        if (read_err != ESP_OK) {
            *peak_mg = 0;
            return read_err;
        }
        s_status.accel_x_mg = accel.x_mg;
        s_status.accel_y_mg = accel.y_mg;
        s_status.accel_z_mg = accel.z_mg;
        int32_t x = s_status.accel_x_mg;
        int32_t y = s_status.accel_y_mg;
        int32_t z = s_status.accel_z_mg;
        uint32_t mag = isqrt_u32((uint32_t)(x * x + y * y + z * z));
        *peak_mg = mag > 1000 ? (uint16_t)(mag - 1000) : (uint16_t)(1000 - mag);
        return ESP_OK;
    }

    uint8_t raw[6] = {0};
    esp_err_t err = s_status.bmi270_spi ?
        bmi270_spi_read(BMI270_ACC_X_LSB_REG, raw, sizeof(raw)) :
        collar_i2c_read_bytes(s_status.bmi270_addr, BMI270_ACC_X_LSB_REG, raw, sizeof(raw));
    if (err != ESP_OK) {
        *peak_mg = 0;
        return err;
    }
    memcpy(s_status.last_acc_raw, raw, sizeof(s_status.last_acc_raw));
    if (s_status.bmi270_spi) {
        bmi270_spi_read(BMI270_STATUS_REG, &s_status.status_reg, 1);
    } else {
        collar_i2c_read_byte(s_status.bmi270_addr, BMI270_STATUS_REG, &s_status.status_reg);
    }

    int16_t x_raw = (int16_t)((uint16_t)raw[1] << 8 | raw[0]);
    int16_t y_raw = (int16_t)((uint16_t)raw[3] << 8 | raw[2]);
    int16_t z_raw = (int16_t)((uint16_t)raw[5] << 8 | raw[4]);
    s_status.accel_x_mg = (int16_t)((int32_t)x_raw * 1000 / 16384);
    s_status.accel_y_mg = (int16_t)((int32_t)y_raw * 1000 / 16384);
    s_status.accel_z_mg = (int16_t)((int32_t)z_raw * 1000 / 16384);

    int32_t x = s_status.accel_x_mg;
    int32_t y = s_status.accel_y_mg;
    int32_t z = s_status.accel_z_mg;
    uint32_t mag = isqrt_u32((uint32_t)(x * x + y * y + z * z));
    *peak_mg = mag > 1000 ? (uint16_t)(mag - 1000) : (uint16_t)(1000 - mag);
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
