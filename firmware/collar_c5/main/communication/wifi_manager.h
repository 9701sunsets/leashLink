#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t collar_wifi_init(void);
esp_err_t collar_wifi_set_credentials(const char *ssid, const char *password);
esp_err_t collar_wifi_connect_saved(void);
bool collar_wifi_is_connected(void);
