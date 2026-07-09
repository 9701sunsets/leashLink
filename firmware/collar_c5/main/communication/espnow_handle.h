#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t espnow_handle_init(void);
esp_err_t espnow_handle_send_telemetry(const ll_collar_telemetry_t *telemetry);
esp_err_t espnow_handle_receive_control_cmd(const ll_control_cmd_t *cmd);
