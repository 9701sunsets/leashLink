#include "i2c_bus.h"

#include "board_pins.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "collar_i2c";
static i2c_master_bus_handle_t s_bus;

esp_err_t collar_i2c_bus_init(void)
{
    if (s_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = COLLAR_SHUTTLE_I2C_PORT,
        .scl_io_num = COLLAR_SHUTTLE_I2C_SCL,
        .sda_io_num = COLLAR_SHUTTLE_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_bus), TAG, "create bus failed");
    ESP_LOGI(TAG, "ready: SDA=GPIO%d SCL=GPIO%d", COLLAR_SHUTTLE_I2C_SDA, COLLAR_SHUTTLE_I2C_SCL);
    return ESP_OK;
}

esp_err_t collar_i2c_probe(uint8_t addr)
{
    ESP_RETURN_ON_ERROR(collar_i2c_bus_init(), TAG, "init failed");
    return i2c_master_probe(s_bus, addr, 100);
}

static esp_err_t add_temp_device(uint8_t addr, i2c_master_dev_handle_t *dev)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    return i2c_master_bus_add_device(s_bus, &dev_config, dev);
}

esp_err_t collar_i2c_read_byte(uint8_t addr, uint8_t reg, uint8_t *value)
{
    if (!value) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(collar_i2c_bus_init(), TAG, "init failed");
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(add_temp_device(addr, &dev), TAG, "add device failed");
    esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, value, 1, 100);
    i2c_master_bus_rm_device(dev);
    return err;
}

esp_err_t collar_i2c_read_bytes(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(collar_i2c_bus_init(), TAG, "init failed");
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(add_temp_device(addr, &dev), TAG, "add device failed");
    esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
    i2c_master_bus_rm_device(dev);
    return err;
}

esp_err_t collar_i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t value)
{
    ESP_RETURN_ON_ERROR(collar_i2c_bus_init(), TAG, "init failed");
    uint8_t data[2] = {reg, value};
    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(add_temp_device(addr, &dev), TAG, "add device failed");
    esp_err_t err = i2c_master_transmit(dev, data, sizeof(data), 100);
    i2c_master_bus_rm_device(dev);
    return err;
}

i2c_master_bus_handle_t collar_i2c_bus_get_handle(void)
{
    return s_bus;
}
