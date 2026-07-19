import { DeviceConfig, DeviceStatus, SafetyEvent, WalkReport } from "../types/index";
import { PAIR_ID } from "./constants";

const now = Date.now();

export const mockStatus: DeviceStatus = {
  pair_id: PAIR_ID,
  online: true,
  last_seen_ms: now,
  active_alert: "none",
  handle: {
    battery_pct: 82,
    tension_n: 8.4,
    tension_peak_n: 22.5,
    tension_stable: true,
    leash_locked: false,
    ambient_light_lux: 228,
    ambient_light_raw: 915,
    dark: true,
    gps: {
      lat: 32.0609,
      lng: 118.778,
      fix: true,
      accuracy_m: 4.5
    }
  },
  collar: {
    battery_pct: 76,
    motion_state: "walk",
    steps: 4521,
    accel_peak_g: 1.4,
    confidence_pct: 80,
    rssi_dbm: -58,
    temp_c_x10: 386,
    distance_est_m: 3.2
  }
};

export const mockEvents: SafetyEvent[] = [
  {
    event_id: "evt-000123",
    type: "burst_pull",
    severity: "danger",
    ts_ms: now - 18 * 60 * 1000,
    source: "handle",
    summary: "检测到突发爆冲，系统已自动锁止牵引结构",
    metrics: {
      tension_peak_n: 22.5,
      accel_peak_g: 2.1,
      response_latency_ms: 310
    },
    actions: ["手柄锁止", "项圈蜂鸣", "项圈震动"]
  },
  {
    event_id: "evt-000122",
    type: "distance_warning",
    severity: "warning",
    ts_ms: now - 88 * 60 * 1000,
    source: "handle",
    summary: "人狗距离超过安全提醒阈值",
    metrics: {
      distance_m: 21
    },
    actions: ["手柄震动", "项圈黄灯提醒"]
  },
  {
    event_id: "evt-000121",
    type: "low_battery",
    severity: "info",
    ts_ms: now - 5 * 60 * 60 * 1000,
    source: "collar",
    summary: "项圈电量低于 20%",
    metrics: {},
    actions: ["降低灯光亮度"]
  }
];

export const mockReport: WalkReport = {
  session_id: "20260709-001",
  duration_min: 52,
  distance_km: 3.2,
  steps: 4521,
  max_tension_n: 22.5,
  burst_count: 2,
  health_state: "healthy",
  tension_series: [5, 8, 7, 10, 12, 9, 14, 22.5, 13, 8, 7, 9]
};

export const mockConfig: DeviceConfig = {
  burst_pull: {
    tension_warn_n: 12,
    tension_lock_n: 20,
    accel_peak_g: 1.8,
    hold_ms: 300
  },
  distance: {
    warn_m: 10,
    danger_m: 20
  },
  night_mode: {
    enabled: true,
    light_threshold_lux: 40
  },
  upload: {
    telemetry_interval_ms: 1000,
    event_immediate: true
  }
};

export const mockTrack = [
  { latitude: 32.0609, longitude: 118.778 },
  { latitude: 32.0611, longitude: 118.7785 },
  { latitude: 32.0615, longitude: 118.779 },
  { latitude: 32.0618, longitude: 118.7795 }
];
