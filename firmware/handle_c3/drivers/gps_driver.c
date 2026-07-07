#include "gps_driver.h"

#include <stdlib.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "board_pins.h"

static const char *TAG = "gps";
static ll_gps_fix_t s_latest;

static double nmea_to_degrees(const char *val, const char hemi)
{
    if (!val || strlen(val) < 4) {
        return 0.0;
    }
    double raw = atof(val);
    int deg = (int)(raw / 100.0);
    double minutes = raw - (double)deg * 100.0;
    double out = (double)deg + minutes / 60.0;
    if (hemi == 'S' || hemi == 'W') {
        out = -out;
    }
    return out;
}

static void parse_gga(char *line)
{
    char *fields[16] = {0};
    uint8_t idx = 0;
    for (char *tok = strtok(line, ","); tok && idx < 16; tok = strtok(NULL, ",")) {
        fields[idx++] = tok;
    }

    if (idx < 10 || (strcmp(fields[0], "$GPGGA") != 0 && strcmp(fields[0], "$GNGGA") != 0)) {
        return;
    }

    int fix_quality = atoi(fields[6]);
    if (fix_quality <= 0) {
        s_latest.fix = false;
        return;
    }

    s_latest.fix = true;
    s_latest.lat = nmea_to_degrees(fields[2], fields[3][0]);
    s_latest.lng = nmea_to_degrees(fields[4], fields[5][0]);
    s_latest.accuracy_m = atof(fields[8]) * 5.0f;
    s_latest.ts_ms = esp_timer_get_time() / 1000;
}

esp_err_t gps_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(HANDLE_GPS_UART_NUM, 2048, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(HANDLE_GPS_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(HANDLE_GPS_UART_NUM, HANDLE_GPS_TX_GPIO, HANDLE_GPS_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t gps_poll(ll_gps_fix_t *out)
{
    static char line[128];
    static size_t len;
    uint8_t c;

    while (uart_read_bytes(HANDLE_GPS_UART_NUM, &c, 1, 0) == 1) {
        if (c == '\n') {
            line[len] = '\0';
            char copy[128];
            strncpy(copy, line, sizeof(copy));
            copy[sizeof(copy) - 1] = '\0';
            parse_gga(copy);
            len = 0;
        } else if (c != '\r' && len < sizeof(line) - 1) {
            line[len++] = (char)c;
        }
    }

    if (out) {
        *out = s_latest;
    }
    return ESP_OK;
}

ll_gps_fix_t gps_get_latest(void)
{
    return s_latest;
}
