#pragma once

#include "esp_err.h"

esp_err_t session_service_init(void);
esp_err_t session_service_start(void);
esp_err_t session_service_stop(void);
const char *session_service_get_id(void);
void session_service_count_event(int event_type);

