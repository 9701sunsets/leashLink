#pragma once

#include "esp_err.h"

esp_err_t mqtt_client_ll_init(void);
esp_err_t mqtt_client_ll_publish(const char *topic, const char *payload, int qos);
