#include "bmi270_espp_bridge.h"

#include <memory>
#include <system_error>
#include <cstring>

#include "bmi270.hpp"
#include "board_pins.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bmi270_bridge";
static constexpr size_t k_bmi270_burst_write_size = 32;

using Bmi270Spi = espp::Bmi270<>;

static spi_device_handle_t s_spi;
static std::unique_ptr<Bmi270Spi> s_imu;
static bool s_ready;
static uint8_t s_pending_read_reg;
static bool s_has_pending_read_reg;
static uint8_t s_spi_tx[k_bmi270_burst_write_size + 8];

static esp_err_t ensure_spi(void)
{
    if (!s_spi) {
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
    }
    return ESP_OK;
}

static void release_spi_device(void)
{
    if (s_spi) {
        spi_bus_remove_device(s_spi);
        s_spi = nullptr;
    }
    s_has_pending_read_reg = false;
}

static bool spi_write_impl(const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
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
    if (len > sizeof(s_spi_tx)) {
        return false;
    }
    memset(s_spi_tx, 0, len);
    s_spi_tx[0] = data[0] & 0x7F;
    if (len > 1) {
        memcpy(&s_spi_tx[1], &data[1], len - 1);
    }
    spi_transaction_t trans = {};
    trans.length = len * 8;
    trans.tx_buffer = s_spi_tx;
    return spi_device_transmit(s_spi, &trans) == ESP_OK;
}

static bool spi_read_impl(uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return false;
    }
    if (!s_has_pending_read_reg) {
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
    memcpy(data, &rx[2], len);
    s_has_pending_read_reg = false;
    return true;
}

static bool spi_write(uint8_t address, const uint8_t *data, size_t len)
{
    (void)address;
    return spi_write_impl(data, len);
}

static bool spi_read(uint8_t address, uint8_t *data, size_t len)
{
    (void)address;
    return spi_read_impl(data, len);
}

esp_err_t collar_bmi270_espp_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "starting BMI270 espp init: SCK=GPIO%d MOSI=GPIO%d MISO=GPIO%d CS=GPIO%d burst=%u",
             COLLAR_SHUTTLE_I2C_SCL,
             COLLAR_SHUTTLE_I2C_SDA,
             COLLAR_BM_SDO_GPIO,
             COLLAR_BM_CS_GPIO,
             (unsigned)k_bmi270_burst_write_size);
    ESP_RETURN_ON_ERROR(ensure_spi(), TAG, "spi failed");

    Bmi270Spi::Config config;
    config.write = spi_write;
    config.read = spi_read;
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

    s_imu = std::make_unique<Bmi270Spi>(config);
    std::error_code ec;
    ESP_LOGI(TAG, "uploading BMI270 config file via espp driver");
    bool ok = s_imu->init(ec);
    if (!ok || ec) {
        ESP_LOGW(TAG, "espp BMI270 init failed: %s", ec.message().c_str());
        s_imu.reset();
        s_ready = false;
        release_spi_device();
        return ESP_ERR_INVALID_RESPONSE;
    }

    s_ready = true;
    ESP_LOGI(TAG, "espp BMI270 initialized");
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
    if (!s_ready || !s_imu) {
        return ESP_ERR_INVALID_STATE;
    }
    std::error_code ec;
    auto accel = s_imu->read_accelerometer(ec);
    if (ec) {
        return ESP_FAIL;
    }
    out->x_g = accel.x;
    out->y_g = accel.y;
    out->z_g = accel.z;
    out->x_mg = static_cast<int16_t>(accel.x * 1000.0f);
    out->y_mg = static_cast<int16_t>(accel.y * 1000.0f);
    out->z_mg = static_cast<int16_t>(accel.z * 1000.0f);
    return ESP_OK;
}
