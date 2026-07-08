from __future__ import annotations

from datetime import datetime, timezone
from typing import Any, Optional

from pydantic import BaseModel, Field


def utc_now_ms() -> int:
    return int(datetime.now(tz=timezone.utc).timestamp() * 1000)


class HardwareInfo(BaseModel):
    handle_mcu: str
    collar_mcu: str
    handle_fw: str
    collar_fw: str


class DeviceRegisterRequest(BaseModel):
    handle_id: str
    collar_id: str
    hardware: HardwareInfo


class DeviceRegisterResponse(BaseModel):
    pair_id: str
    mqtt_topic_prefix: str
    created_at: int


class GPSLocation(BaseModel):
    lat: float
    lng: float
    fix: bool = True
    accuracy_m: Optional[float] = None


class HandleStatus(BaseModel):
    battery_pct: int = 0
    tension_n: float = 0.0
    leash_locked: bool = False
    gps: Optional[GPSLocation] = None


class CollarStatus(BaseModel):
    battery_pct: int = 0
    motion_state: str = "unknown"
    steps: int = 0
    rssi_dbm: Optional[int] = None
    distance_est_m: Optional[float] = None


class DeviceStatusResponse(BaseModel):
    pair_id: str
    online: bool = False
    last_seen_ms: Optional[int] = None
    handle: HandleStatus
    collar: CollarStatus
    active_alert: str = "none"


class HandleTelemetryInput(BaseModel):
    tension_n: Optional[float] = None
    leash_length_m: Optional[float] = None
    leash_locked: Optional[bool] = None
    ambient_light_lux: Optional[float] = None
    battery_pct: Optional[int] = None
    gps: Optional[GPSLocation] = None


class CollarTelemetryInput(BaseModel):
    motion_state: Optional[str] = None
    steps: Optional[int] = None
    accel_peak_g: Optional[float] = None
    confidence_pct: Optional[int] = None
    battery_pct: Optional[int] = None
    rssi_dbm: Optional[int] = None
    temp_c_x10: Optional[int] = None
    distance_est_m: Optional[float] = None


class TelemetryUpsertRequest(BaseModel):
    pair_id: str
    session_id: str
    ts_ms: int = Field(default_factory=utc_now_ms)
    handle: HandleTelemetryInput
    collar: CollarTelemetryInput
    location: Optional[GPSLocation] = None
    alert: str = "none"


class TelemetryResponse(BaseModel):
    accepted: bool = True
    server_ts_ms: int = Field(default_factory=utc_now_ms)


class EventMetrics(BaseModel):
    tension_peak_n: Optional[float] = None
    accel_peak_g: Optional[float] = None
    response_latency_ms: Optional[int] = None
    distance_est_m: Optional[float] = None


class EventCreateRequest(BaseModel):
    pair_id: str
    session_id: str
    event_id: str
    type: str
    severity: str
    ts_ms: int = Field(default_factory=utc_now_ms)
    source: Optional[str] = None
    metrics: EventMetrics = Field(default_factory=EventMetrics)
    actions: list[str] = Field(default_factory=list)


class EventSummary(BaseModel):
    event_id: str
    type: str
    severity: str
    ts_ms: int
    summary: str


class EventListResponse(BaseModel):
    items: list[EventSummary]
    next_cursor: Optional[str] = None


class FenceCenter(BaseModel):
    lat: float
    lng: float


class FenceConfigRequest(BaseModel):
    enabled: bool
    mode: str = "circle"
    center: FenceCenter
    radius_m: float
    warn_margin_m: float


class FenceConfigResponse(BaseModel):
    saved: bool = True
    version: int


class BurstPullConfig(BaseModel):
    tension_warn_n: float
    tension_lock_n: float
    accel_peak_g: float
    hold_ms: int


class DistanceConfig(BaseModel):
    warn_m: float
    danger_m: float


class NightModeConfig(BaseModel):
    enabled: bool
    light_threshold_lux: float


class UploadConfig(BaseModel):
    telemetry_interval_ms: int
    event_immediate: bool


class DeviceConfigRequest(BaseModel):
    burst_pull: BurstPullConfig
    distance: DistanceConfig
    night_mode: NightModeConfig
    upload: UploadConfig


class DeviceConfigResponse(BaseModel):
    saved: bool = True
    config_version: int


class CommandAck(BaseModel):
    cmd_id: str
    accepted: bool
    node: str
    code: str
    message: str
    ts_ms: int = Field(default_factory=utc_now_ms)


class DeviceRecord(BaseModel):
    pair_id: str
    handle_id: str
    collar_id: str
    handle_mac: Optional[str] = None
    collar_mac: Optional[str] = None
    protocol_version: int = 1
    created_at: int = Field(default_factory=utc_now_ms)
    hardware: HardwareInfo


class StoredTelemetry(TelemetryUpsertRequest):
    pass


class StoredEvent(EventCreateRequest):
    pass


class StatusSnapshot(BaseModel):
    pair_id: str
    online: bool
    last_seen_ms: Optional[int]
    handle: HandleStatus
    collar: CollarStatus
    active_alert: str


class TopicEnvelope(BaseModel):
    topic: str
    payload: dict[str, Any]
