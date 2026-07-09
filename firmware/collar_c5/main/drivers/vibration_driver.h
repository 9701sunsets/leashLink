#pragma once

#include "esp_err.h"

esp_err_t vibration_init(void);
esp_err_t vibration_run(uint16_t duration_ms);
esp_err_t vibration_stop(void);
