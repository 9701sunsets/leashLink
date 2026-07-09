#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t feedback_service_init(void);
esp_err_t feedback_service_handle_cmd(const ll_control_cmd_t *cmd);
