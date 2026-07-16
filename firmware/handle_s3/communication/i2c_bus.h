#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_probe(uint8_t addr);
esp_err_t i2c_bus_write_byte(uint8_t addr, uint8_t reg, uint8_t value);
esp_err_t i2c_bus_read_byte(uint8_t addr, uint8_t reg, uint8_t *value);
esp_err_t i2c_bus_read_bytes(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);
i2c_master_bus_handle_t i2c_bus_get_handle(void);
