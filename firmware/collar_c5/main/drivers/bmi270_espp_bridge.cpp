#include "bmi270_espp_bridge.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <system_error>

#include "bmi270.hpp"
#include "board_pins.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "i2c_bus.h"

static const char *TAG = "bmi270_bridge";
static uint8_t s_bmi270_i2c_addr = 0x68;
static constexpr size_t k_bmi270_burst_write_size = 2;
static constexpr size_t k_spi_max_transfer_size = 64;

using Bmi270I2c = espp::Bmi270<espp::bmi270::Interface::I2C>;
using Bmi270Spi = espp::Bmi270<espp::bmi270::Interface::SPI>;

static std::unique_ptr<Bmi270I2c> s_imu_i2c;
static std::unique_ptr<Bmi270Spi> s_imu_spi;
static spi_device_handle_t s_spi;
static bool s_ready;
static bool s_ready_spi;
static uint8_t s_pending_read_reg;
static bool s_has_pending_read_reg;
static uint8_t s_spi_tx[k_spi_max_transfer_size];

static void reset_ready_state(void)
{
    s_ready = false;
    s_ready_spi = false;
    s_imu_i2c.reset();
    s_imu_spi.reset();
    s_has_pending_read_reg = false;
}

static esp_err_t ensure_i2c(void)
{
    return collar_i2c_bus_init();
}

static bool i2c_write_impl(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return false;
    }
    if (ensure_i2c() != ESP_OK) {
        return false;
    }
    if (len == 1) {
        s_pending_read_reg = data[0];
        s_has_pending_read_reg = true;
        return true;
    }
    return collar_i2c_write_bytes(s_bmi270_i2c_addr, data, len) == ESP_OK;
}

static bool i2c_read_impl(uint8_t *data, size_t len)
{
    if (!data || len == 0 || !s_has_pending_read_reg) {
        return false;
    }
    if (ensure_i2c() != ESP_OK) {
        return false;
    }
    esp_err_t err = collar_i2c_read_bytes(s_bmi270_i2c_addr, s_pending_read_reg, data, len);
    s_has_pending_read_reg = false;
    return err == ESP_OK;
}

static bool i2c_write(uint8_t address, const uint8_t *data, size_t len)
{
    (void)address;
    return i2c_write_impl(data, len);
}

static bool i2c_read(uint8_t address, uint8_t *data, size_t len)
{
    (void)address;
    return i2c_read_impl(data, len);
}

static esp_err_t ensure_spi(void)
{
    if (s_spi) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(collar_i2c_bus_deinit(), TAG, "release i2c failed");

    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = COLLAR_SHUTTLE_I2C_SDA;
    buscfg.miso_io_num = COLLAR_BM_SDO_GPIO;
    buscfg.sclk_io_num = COLLAR_SHUTTLE_I2C_SCL;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = sizeof(s_spi_tx);

    esp_err_t err = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "spi bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;
    devcfg.mode = 0;
    devcfg.clock_speed_hz = 1000 * 1000;
    devcfg.spics_io_num = COLLAR_BM_CS_GPIO;
    devcfg.queue_size = 1;
    err = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "spi add device failed: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
}

static void release_spi(void)
{
    if (s_spi) {
        spi_bus_remove_device(s_spi);
        s_spi = nullptr;
    }
    spi_bus_free(SPI2_HOST);
}

static bool spi_write_impl(const uint8_t *data, size_t len)
{
    if (!data || len == 0 || len > sizeof(s_spi_tx)) {
        return false;
    }
    if (ensure_spi() != ESP_OK) {
        return false;
    }
    if (len == 1) {
        s_pending_read_reg = data[0];
        s_has_pending_read_reg = true;
        return true;
    }
    std::memset(s_spi_tx, 0, len);
    s_spi_tx[0] = data[0] & 0x7f;
    if (len > 1) {
        std::memcpy(&s_spi_tx[1], &data[1], len - 1);
    }
    spi_transaction_t trans = {};
    trans.length = len * 8;
    trans.tx_buffer = s_spi_tx;
    esp_err_t err = spi_device_transmit(s_spi, &trans);
    if (err == ESP_OK && data[0] == 0x5e) {
        esp_rom_delay_us(450);
    }
    return err == ESP_OK;
}

static bool spi_read_impl(uint8_t *data, size_t len)
{
    if (!data || len == 0 || !s_has_pending_read_reg) {
        return false;
    }
    if (ensure_spi() != ESP_OK) {
        return false;
    }
    uint8_t tx[64] = {};
    uint8_t rx[64] = {};
    size_t total_len = len + 2;
    if (total_len > sizeof(tx)) {
        return false;
    }
    tx[0] = s_pending_read_reg | 0x80;
    spi_transaction_t trans = {};
    trans.length = total_len * 8;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;
    if (spi_device_transmit(s_spi, &trans) != ESP_OK) {
        return false;
    }
    std::memcpy(data, &rx[2], len);
    s_has_pending_read_reg = false;
    return true;
}

static bool spi_read_reg_raw(uint8_t reg, uint8_t *data, size_t len)
{
    s_pending_read_reg = reg;
    s_has_pending_read_reg = true;
    return spi_read_impl(data, len);
}

static esp_err_t manual_spi_write_reg(uint8_t reg, const uint8_t *data, size_t len)
{
    if (!data && len > 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (len + 1 > sizeof(s_spi_tx)) {
        return ESP_ERR_INVALID_SIZE;
    }
    ESP_RETURN_ON_ERROR(ensure_spi(), TAG, "spi failed");
    s_spi_tx[0] = reg & 0x7f;
    if (len > 0) {
        std::memcpy(&s_spi_tx[1], data, len);
    }
    spi_transaction_t trans = {};
    trans.length = (len + 1) * 8;
    trans.tx_buffer = s_spi_tx;
    esp_err_t err = spi_device_transmit(s_spi, &trans);
    if (err == ESP_OK && reg == 0x5e) {
        esp_rom_delay_us(450);
    }
    return err;
}

static esp_err_t manual_spi_write_u8(uint8_t reg, uint8_t value)
{
    return manual_spi_write_reg(reg, &value, 1);
}

static esp_err_t manual_spi_read_reg(uint8_t reg, uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(ensure_spi(), TAG, "spi failed");
    uint8_t tx[64] = {};
    uint8_t rx[64] = {};
    size_t total_len = len + 2;
    if (total_len > sizeof(tx)) {
        return ESP_ERR_INVALID_SIZE;
    }
    tx[0] = reg | 0x80;
    spi_transaction_t trans = {};
    trans.length = total_len * 8;
    trans.tx_buffer = tx;
    trans.rx_buffer = rx;
    ESP_RETURN_ON_ERROR(spi_device_transmit(s_spi, &trans), TAG, "spi read failed");
    std::memcpy(data, &rx[2], len);
    return ESP_OK;
}

static esp_err_t manual_spi_wait_chip(uint8_t *chip, const char *phase)
{
    if (!chip) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t value = 0xff;
    for (int i = 0; i < 40; ++i) {
        esp_rom_delay_us(5000);
        (void)manual_spi_read_reg(0x00, &value, 1);
        if (value == 0x24) {
            *chip = value;
            if (i > 0) {
                ESP_LOGI(TAG, "BMI270 SPI chip ready after %s retry=%d", phase, i);
            }
            return ESP_OK;
        }
    }
    *chip = value;
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t manual_bmi270_config_upload_spi(void)
{
    const uint8_t *config = Bmi270Spi::get_config_file();
    const size_t config_size = Bmi270Spi::get_config_file_size();
    uint8_t chip = 0;
    ESP_RETURN_ON_ERROR(manual_spi_read_reg(0x00, &chip, 1), TAG, "read chip before reset failed");
    if (chip != 0x24) {
        ESP_LOGW(TAG, "manual BMI270 SPI upload invalid chip before reset: 0x%02x", chip);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x7e, 0xb6), TAG, "soft reset failed");
    esp_err_t chip_err = manual_spi_wait_chip(&chip, "soft reset");
    if (chip_err != ESP_OK) {
        ESP_LOGW(TAG, "manual BMI270 SPI upload invalid chip after reset: 0x%02x", chip);
        return chip_err;
    }

    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x7c, 0x00), TAG, "disable aps failed");
    esp_rom_delay_us(1000);
    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x59, 0x00), TAG, "prepare config failed");
    esp_rom_delay_us(1000);

    for (size_t offset = 0; offset < config_size; offset += k_bmi270_burst_write_size) {
        size_t chunk = std::min(k_bmi270_burst_write_size, config_size - offset);
        uint16_t init_addr = static_cast<uint16_t>(offset / 2);
        uint8_t addr_data[2] = {
            static_cast<uint8_t>(init_addr & 0x0f),
            static_cast<uint8_t>(init_addr >> 4),
        };
        ESP_RETURN_ON_ERROR(manual_spi_write_reg(0x5b, addr_data, sizeof(addr_data)),
                            TAG,
                            "write init addr failed");
        ESP_RETURN_ON_ERROR(manual_spi_write_reg(0x5e, config + offset, chunk),
                            TAG,
                            "write init data failed");
    }

    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x59, 0x01), TAG, "finish config failed");
    uint8_t err_reg = 0;
    uint8_t internal_status = 0;
    uint8_t init_ctrl = 0;
    for (int i = 0; i < 200; ++i) {
        esp_rom_delay_us(1000);
        manual_spi_read_reg(0x21, &internal_status, 1);
        if ((internal_status & 0x01) != 0) {
            break;
        }
    }
    manual_spi_read_reg(0x02, &err_reg, 1);
    manual_spi_read_reg(0x59, &init_ctrl, 1);
    ESP_LOGI(TAG, "manual BMI270 SPI upload status: err=0x%02x internal=0x%02x init_ctrl=0x%02x",
             err_reg,
             internal_status,
             init_ctrl);
    if ((internal_status & 0x01) == 0) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x40, 0xa8), TAG, "write acc conf failed");
    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x41, 0x01), TAG, "write acc range failed");
    ESP_RETURN_ON_ERROR(manual_spi_write_u8(0x7d, 0x04), TAG, "enable accel failed");
    esp_rom_delay_us(20000);
    return ESP_OK;
}

static Bmi270I2c::Config common_i2c_config(void)
{
    Bmi270I2c::Config config;
    config.device_address = s_bmi270_i2c_addr;
    config.write = i2c_write;
    config.read = i2c_read;
    config.imu_config.accelerometer_range = Bmi270I2c::AccelerometerRange::RANGE_4G;
    config.imu_config.accelerometer_odr = Bmi270I2c::AccelerometerODR::ODR_100_HZ;
    config.imu_config.accelerometer_bandwidth = Bmi270I2c::AccelerometerBandwidth::NORMAL_AVG4;
    config.imu_config.gyroscope_range = Bmi270I2c::GyroscopeRange::RANGE_2000DPS;
    config.imu_config.gyroscope_odr = Bmi270I2c::GyroscopeODR::ODR_100_HZ;
    config.imu_config.gyroscope_bandwidth = Bmi270I2c::GyroscopeBandwidth::NORMAL_MODE;
    config.imu_config.gyroscope_performance_mode = Bmi270I2c::GyroscopePerformanceMode::POWER_OPTIMIZED;
    config.imu_config.enable_advanced_features = false;
    config.imu_config.fifo_mode = Bmi270I2c::FifoMode::BYPASS;
    config.burst_write_size = k_bmi270_burst_write_size;
    config.auto_init = false;
    return config;
}

static Bmi270Spi::Config common_spi_config(void)
{
    Bmi270Spi::Config config;
    config.write = spi_write_impl;
    config.read = spi_read_impl;
    config.imu_config.accelerometer_range = Bmi270Spi::AccelerometerRange::RANGE_4G;
    config.imu_config.accelerometer_odr = Bmi270Spi::AccelerometerODR::ODR_100_HZ;
    config.imu_config.accelerometer_bandwidth = Bmi270Spi::AccelerometerBandwidth::NORMAL_AVG4;
    config.imu_config.gyroscope_range = Bmi270Spi::GyroscopeRange::RANGE_2000DPS;
    config.imu_config.gyroscope_odr = Bmi270Spi::GyroscopeODR::ODR_100_HZ;
    config.imu_config.gyroscope_bandwidth = Bmi270Spi::GyroscopeBandwidth::NORMAL_MODE;
    config.imu_config.gyroscope_performance_mode = Bmi270Spi::GyroscopePerformanceMode::POWER_OPTIMIZED;
    config.imu_config.enable_advanced_features = false;
    config.imu_config.fifo_mode = Bmi270Spi::FifoMode::BYPASS;
    config.burst_write_size = k_bmi270_burst_write_size;
    config.auto_init = false;
    return config;
}

esp_err_t collar_bmi270_espp_init(void)
{
    return collar_bmi270_espp_init_i2c(s_bmi270_i2c_addr);
}

esp_err_t collar_bmi270_espp_init_i2c(uint8_t addr)
{
    if (s_ready && !s_ready_spi) {
        return ESP_OK;
    }
    reset_ready_state();
    s_bmi270_i2c_addr = addr;
    ESP_LOGI(TAG, "starting BMI270 espp init over I2C: SDA=GPIO%d SCL=GPIO%d CS=GPIO%d:1 addr=0x%02x burst=%u",
             COLLAR_SHUTTLE_I2C_SDA,
             COLLAR_SHUTTLE_I2C_SCL,
             COLLAR_BM_CS_GPIO,
             s_bmi270_i2c_addr,
             (unsigned)k_bmi270_burst_write_size);
    ESP_RETURN_ON_ERROR(ensure_i2c(), TAG, "i2c failed");

    s_imu_i2c = std::make_unique<Bmi270I2c>(common_i2c_config());
    std::error_code ec;
    ESP_LOGI(TAG, "uploading BMI270 config file via espp I2C driver");
    bool ok = s_imu_i2c->init(ec);
    if (!ok || ec) {
        uint8_t err_reg = 0;
        uint8_t init_ctrl = 0;
        uint8_t internal_status = 0;
        s_pending_read_reg = 0x02;
        s_has_pending_read_reg = true;
        i2c_read_impl(&err_reg, 1);
        s_pending_read_reg = 0x21;
        s_has_pending_read_reg = true;
        i2c_read_impl(&internal_status, 1);
        s_pending_read_reg = 0x59;
        s_has_pending_read_reg = true;
        i2c_read_impl(&init_ctrl, 1);
        ESP_LOGW(TAG, "BMI270 status after I2C config failure: err=0x%02x internal=0x%02x init_ctrl=0x%02x",
                 err_reg,
                 internal_status,
                 init_ctrl);
        ESP_LOGW(TAG, "espp BMI270 I2C init failed: %s", ec.message().c_str());
        reset_ready_state();
        return ESP_ERR_INVALID_RESPONSE;
    }

    s_ready = true;
    s_ready_spi = false;
    ESP_LOGI(TAG, "espp BMI270 initialized over I2C");
    return ESP_OK;
}

esp_err_t collar_bmi270_espp_init_spi(void)
{
    if (s_ready && s_ready_spi) {
        return ESP_OK;
    }
    reset_ready_state();
    ESP_LOGI(TAG, "starting BMI270 espp init over SPI: SCK=GPIO%d MOSI=GPIO%d MISO=GPIO%d CS=GPIO%d burst=%u",
             COLLAR_SHUTTLE_I2C_SCL,
             COLLAR_SHUTTLE_I2C_SDA,
             COLLAR_BM_SDO_GPIO,
             COLLAR_BM_CS_GPIO,
             (unsigned)k_bmi270_burst_write_size);
    ESP_RETURN_ON_ERROR(ensure_spi(), TAG, "spi failed");

    ESP_LOGI(TAG, "uploading BMI270 config file via manual SPI driver");
    esp_err_t upload_err = manual_bmi270_config_upload_spi();
    if (upload_err != ESP_OK) {
        uint8_t err_reg = 0;
        uint8_t init_ctrl = 0;
        uint8_t internal_status = 0;
        manual_spi_read_reg(0x02, &err_reg, 1);
        manual_spi_read_reg(0x21, &internal_status, 1);
        manual_spi_read_reg(0x59, &init_ctrl, 1);
        ESP_LOGW(TAG, "BMI270 status after SPI config failure: err=0x%02x internal=0x%02x init_ctrl=0x%02x",
                 err_reg,
                 internal_status,
                 init_ctrl);
        ESP_LOGW(TAG, "manual BMI270 SPI init failed: %s", esp_err_to_name(upload_err));
        reset_ready_state();
        release_spi();
        return upload_err;
    }

    s_ready = true;
    s_ready_spi = true;
    ESP_LOGI(TAG, "BMI270 initialized over manual SPI");
    return ESP_OK;
}

bool collar_bmi270_espp_is_ready(void)
{
    return s_ready;
}

esp_err_t collar_bmi270_espp_read_accel(collar_bmi270_accel_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    std::error_code ec;
    Bmi270Spi::Value accel = {};
    if (s_ready_spi) {
        if (s_imu_spi) {
            accel = s_imu_spi->read_accelerometer(ec);
            if (ec) {
                return ESP_FAIL;
            }
        } else {
            uint8_t raw[6] = {};
            ESP_RETURN_ON_ERROR(manual_spi_read_reg(0x0c, raw, sizeof(raw)), TAG, "read accel failed");
            int16_t x_raw = static_cast<int16_t>((static_cast<uint16_t>(raw[1]) << 8) | raw[0]);
            int16_t y_raw = static_cast<int16_t>((static_cast<uint16_t>(raw[3]) << 8) | raw[2]);
            int16_t z_raw = static_cast<int16_t>((static_cast<uint16_t>(raw[5]) << 8) | raw[4]);
            accel.x = static_cast<float>(x_raw) / 8192.0f;
            accel.y = static_cast<float>(y_raw) / 8192.0f;
            accel.z = static_cast<float>(z_raw) / 8192.0f;
        }
    } else {
        if (!s_imu_i2c) {
            return ESP_ERR_INVALID_STATE;
        }
        auto i2c_accel = s_imu_i2c->read_accelerometer(ec);
        accel.x = i2c_accel.x;
        accel.y = i2c_accel.y;
        accel.z = i2c_accel.z;
    }
    out->x_g = accel.x;
    out->y_g = accel.y;
    out->z_g = accel.z;
    out->x_mg = static_cast<int16_t>(accel.x * 1000.0f);
    out->y_mg = static_cast<int16_t>(accel.y * 1000.0f);
    out->z_mg = static_cast<int16_t>(accel.z * 1000.0f);
    return ESP_OK;
}
