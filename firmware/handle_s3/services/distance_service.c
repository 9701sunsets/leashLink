#include "distance_service.h"

#include <math.h>

#define DEFAULT_TX_POWER_DBM (-59)
#define PATH_LOSS_N 2.2f

static double s_fence_lat = 32.0609;// 纬度
static double s_fence_lng = 118.7780;// 经度
static float s_fence_radius_m = 100.0f;// 半径（米）
static bool s_fence_enabled = false;// 围栏是否启用

/**
 * 将角度转换为弧度
 */
static double deg2rad(double deg)
{
    return deg * 3.14159265358979323846 / 180.0;
}

/**
 * 计算两点之间的Haversine距离（米）
 */
static float haversine_m(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = deg2rad(lat2 - lat1);
    double dlon = deg2rad(lon2 - lon1);
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
               sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return (float)(6371000.0 * c);
}

/**
 * 初始化距离服务
 */
esp_err_t distance_service_init(void)
{
    return ESP_OK;
}

/**
 * 设置GPS围栏
 * @param lat 纬度
 * @param lng 经度
 * @param radius_m 半径（米）
 */
float distance_service_estimate_from_rssi(int8_t rssi_dbm)
{
    if (rssi_dbm >= 0 || rssi_dbm < -120) {
        return 999.0f;
    }
    return powf(10.0f, ((float)DEFAULT_TX_POWER_DBM - (float)rssi_dbm) / (10.0f * PATH_LOSS_N));
}

/**
 * 设置GPS围栏
 * @param lat 纬度
 * @param lng 经度
 * @param radius_m 半径（米）
 */
bool distance_service_is_fence_breach(const ll_gps_fix_t *fix)
{
    if (!s_fence_enabled || !fix || !fix->fix || fix->accuracy_m > 30.0f) {
        return false;
    }

    return haversine_m(s_fence_lat, s_fence_lng, fix->lat, fix->lng) > s_fence_radius_m;
}

