export type MotionState = "idle" | "walk" | "run" | "burst" | "shake" | "unknown";
export type AlertType = "none" | "burst_pull" | "distance_warning" | "fence_breach" | "fatigue_detected" | "low_battery" | "link_lost";
export type Severity = "info" | "warning" | "danger";

export interface GpsFix {
  lat: number;
  lng: number;
  fix: boolean;
  accuracy_m: number;
}

export interface DeviceStatus {
  pair_id: string;
  online: boolean;
  last_seen_ms: number;
  active_alert: AlertType;
  handle: {
    battery_pct: number;
    tension_n: number;
    leash_locked: boolean;
    gps: GpsFix;
  };
  collar: {
    battery_pct: number;
    motion_state: MotionState;
    steps: number;
    rssi_dbm: number;
    distance_est_m: number;
  };
}

export interface SafetyEvent {
  event_id: string;
  type: AlertType;
  severity: Severity;
  ts_ms: number;
  source?: "handle" | "collar" | "cloud";
  summary: string;
  metrics: {
    tension_peak_n?: number;
    accel_peak_g?: number;
    response_latency_ms?: number;
    distance_m?: number;
  };
  actions: string[];
}

export interface WalkReport {
  session_id: string;
  duration_min: number;
  distance_km: number;
  steps: number;
  max_tension_n: number;
  burst_count: number;
  health_state: "healthy" | "tired" | "attention";
  tension_series: number[];
}

export interface DeviceConfig {
  burst_pull: {
    tension_warn_n: number;
    tension_lock_n: number;
    accel_peak_g: number;
    hold_ms: number;
  };
  distance: {
    warn_m: number;
    danger_m: number;
  };
  night_mode: {
    enabled: boolean;
    light_threshold_lux: number;
  };
  upload: {
    telemetry_interval_ms: number;
    event_immediate: boolean;
  };
}

