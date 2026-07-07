#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t motor_init(void);
esp_err_t motor_vibrate(uint16_t duration_ms);
esp_err_t motor_stop(void);

