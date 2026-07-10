#include "session_service.h"

#include <stdio.h>

#include "esp_timer.h"

static char s_session_id[32] = "idle";
static bool s_active;
static uint32_t s_burst_count;
static uint32_t s_distance_count;

esp_err_t session_service_init(void)
{
    return session_service_start();
}

esp_err_t session_service_start(void)
{
    snprintf(s_session_id, sizeof(s_session_id), "S-%lld", esp_timer_get_time() / 1000);
    s_active = true;
    s_burst_count = 0;
    s_distance_count = 0;
    return ESP_OK;
}

esp_err_t session_service_stop(void)
{
    s_active = false;
    return ESP_OK;
}

const char *session_service_get_id(void)
{
    return s_active ? s_session_id : "idle";
}

void session_service_count_event(int event_type)
{
    if (event_type == 1) {
        s_burst_count++;
    } else if (event_type == 2 || event_type == 3) {
        s_distance_count++;
    }
}

