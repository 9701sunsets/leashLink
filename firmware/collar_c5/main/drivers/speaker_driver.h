#pragma once

#include <stdint.h>

#include "esp_err.h"

esp_err_t speaker_init(void);
esp_err_t speaker_beep(uint16_t freq_hz, uint16_t duration_ms);
esp_err_t speaker_stop(void);
