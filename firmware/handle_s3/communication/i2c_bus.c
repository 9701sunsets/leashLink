#include "i2c_bus.h"

#include "board_pins.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "i2c_bus";
static i2c_master_bus_handle_t s_bus;

esp_err_t i2c_bus_init(void)
{
    if (s_bus) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = HANDLE_I2C_PORT_NUM,
        .scl_io_num = HANDLE_I2C_SCL_GPIO,
        .sda_io_num = HANDLE_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_bus), TAG, "create I2C bus failed");
    ESP_LOGI(TAG, "I2C ready: SDA=GPIO%d SCL=GPIO%d",
             HANDLE_I2C_SDA_GPIO, HANDLE_I2C_SCL_GPIO);
    return ESP_OK;
}

esp_err_t i2c_bus_probe(uint8_t addr)
{
    ESP_RETURN_ON_ERROR(i2c_bus_init(), TAG, "I2C init failed");
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

esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(i2c_bus_init(), TAG, "I2C init failed");

    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(add_temp_device(addr, &dev), TAG, "add I2C device failed");
    esp_err_t err = i2c_master_transmit(dev, data, len, 100);
    i2c_master_bus_rm_device(dev);
    return err;
}

esp_err_t i2c_bus_write_byte(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_bus_write(addr, data, sizeof(data));
}

esp_err_t i2c_bus_read_bytes(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(i2c_bus_init(), TAG, "I2C init failed");

    i2c_master_dev_handle_t dev;
    ESP_RETURN_ON_ERROR(add_temp_device(addr, &dev), TAG, "add I2C device failed");
    esp_err_t err = i2c_master_transmit_receive(dev, &reg, 1, data, len, 100);
    i2c_master_bus_rm_device(dev);
    return err;
}

esp_err_t i2c_bus_read_byte(uint8_t addr, uint8_t reg, uint8_t *value)
{
    return i2c_bus_read_bytes(addr, reg, value, 1);
}

i2c_master_bus_handle_t i2c_bus_get_handle(void)
{
    return s_bus;
}
