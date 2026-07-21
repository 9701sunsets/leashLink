from __future__ import annotations

from datetime import datetime, timezone
from typing import Any, Optional

from pydantic import BaseModel, Field

# 获取当前UTC时间戳（毫秒）
def utc_now_ms() -> int:
    return int(datetime.now(tz=timezone.utc).timestamp() * 1000)

# 定义数据模型
class HardwareInfo(BaseModel):
    handle_mcu: str
    collar_mcu: str
    handle_fw: str
    collar_fw: str

# 设备注册请求和响应模型
class DeviceRegisterRequest(BaseModel):
    handle_id: str
    collar_id: str
    hardware: HardwareInfo

# 设备注册响应模型
class DeviceRegisterResponse(BaseModel):
    pair_id: str
    mqtt_topic_prefix: str
    created_at: int

# 设备状态和遥测数据模型
class GPSLocation(BaseModel):
    lat: float
    lng: float
    fix: bool = True
    accuracy_m: Optional[float] = None


class HeartSensorStatus(BaseModel):
    present: bool = False
    ok: bool = False
    ir: Optional[int] = None
    red: Optional[int] = None
    i2c_addr: Optional[int] = None
    part_id: Optional[int] = None
    int_level: Optional[int] = None

# 设备状态响应模型
class HandleStatus(BaseModel):
    battery_pct: int = 0
    tension_n: float = 0.0
    tension_peak_n: float = 0.0
    tension_stable: bool = False
    leash_locked: bool = False
    ambient_light_lux: Optional[float] = None
    ambient_light_raw: Optional[int] = None
    dark: Optional[bool] = None
    heart: HeartSensorStatus = Field(default_factory=HeartSensorStatus)
    gps: Optional[GPSLocation] = None

# 设备状态响应模型
class CollarStatus(BaseModel):
    battery_pct: int = 0
    motion_state: str = "unknown"
    steps: int = 0
    accel_peak_g: Optional[float] = None
    confidence_pct: Optional[int] = None
    rssi_dbm: Optional[int] = None
    temp_c_x10: Optional[int] = None
    distance_est_m: Optional[float] = None

# 设备状态响应模型
class DeviceStatusResponse(BaseModel):
    pair_id: str
    online: bool = False
    last_seen_ms: Optional[int] = None
    handle: HandleStatus
    collar: CollarStatus
    active_alert: str = "none"


class DogProfile(BaseModel):
    id: int
    pair_id: str
    name: str
    owner: str = ""
    breed: str = "mixed"
    age: float = 2
    weight: float = 10
    neutered: bool = False
    calories_now: int = 0
    walk_minutes: int = 0
    distance_km: float = 0.0
    created_at: int = Field(default_factory=utc_now_ms)
    updated_at: int = Field(default_factory=utc_now_ms)


class DogProfileCreateRequest(BaseModel):
    name: str
    owner: str = ""
    breed: str = "mixed"
    age: float = 2
    weight: float = 10
    neutered: bool = False
    calories_now: int = 0
    walk_minutes: int = 0
    distance_km: float = 0.0


class DogProfileUpdateRequest(BaseModel):
    name: Optional[str] = None
    owner: Optional[str] = None
    breed: Optional[str] = None
    age: Optional[float] = None
    weight: Optional[float] = None
    neutered: Optional[bool] = None
    calories_now: Optional[int] = None
    walk_minutes: Optional[int] = None
    distance_km: Optional[float] = None


class DogProfileListResponse(BaseModel):
    items: list[DogProfile]

# 设备遥测数据模型
class HandleTelemetryInput(BaseModel):
    tension_n: Optional[float] = None
    tension_peak_n: Optional[float] = None
    tension_stable: Optional[bool] = None
    leash_length_m: Optional[float] = None
    leash_locked: Optional[bool] = None
    ambient_light_lux: Optional[float] = None
    ambient_light_raw: Optional[int] = None
    dark: Optional[bool] = None
    heart: Optional[HeartSensorStatus] = None
    battery_pct: Optional[int] = None
    gps: Optional[GPSLocation] = None

# 设备遥测数据模型
class CollarTelemetryInput(BaseModel):
    motion_state: Optional[str] = None
    steps: Optional[int] = None
    accel_peak_g: Optional[float] = None
    confidence_pct: Optional[int] = None
    battery_pct: Optional[int] = None
    rssi_dbm: Optional[int] = None
    temp_c_x10: Optional[int] = None
    distance_est_m: Optional[float] = None

# 设备遥测数据上报请求和响应模型
class TelemetryUpsertRequest(BaseModel):
    pair_id: str
    session_id: str
    ts_ms: int = Field(default_factory=utc_now_ms)
    handle: HandleTelemetryInput
    collar: CollarTelemetryInput
    location: Optional[GPSLocation] = None
    alert: str = "none"

# 设备遥测数据上报响应模型
class TelemetryResponse(BaseModel):
    accepted: bool = True
    server_ts_ms: int = Field(default_factory=utc_now_ms)

# 事件数据模型
class EventMetrics(BaseModel):
    tension_peak_n: Optional[float] = None
    accel_peak_g: Optional[float] = None
    response_latency_ms: Optional[int] = None
    distance_est_m: Optional[float] = None

# 事件创建请求和响应模型
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

# 事件创建响应模型
class EventSummary(BaseModel):
    event_id: str
    type: str
    severity: str
    ts_ms: int
    summary: str

# 事件列表响应模型
class EventListResponse(BaseModel):
    items: list[EventSummary]
    next_cursor: Optional[str] = None


class WalkReportResponse(BaseModel):
    session_id: str
    duration_min: int = 0
    distance_km: float = 0.0
    steps: int = 0
    max_tension_n: float = 0.0
    burst_count: int = 0
    health_state: str = "healthy"
    tension_series: list[float] = Field(default_factory=list)

# 设备围栏配置模型
class FenceCenter(BaseModel):
    lat: float
    lng: float

# 设备围栏配置请求和响应模型
class FenceConfigRequest(BaseModel):
    enabled: bool
    mode: str = "circle"
    center: FenceCenter
    radius_m: float
    warn_margin_m: float

# 设备围栏配置响应模型
class FenceConfigResponse(BaseModel):
    saved: bool = True
    version: int

# 设备配置模型
class BurstPullConfig(BaseModel):
    tension_warn_n: float
    tension_lock_n: float
    accel_peak_g: float
    hold_ms: int

# 设备配置模型
class DistanceConfig(BaseModel):
    warn_m: float
    danger_m: float

# 设备配置模型
class NightModeConfig(BaseModel):
    enabled: bool
    light_threshold_lux: float

# 设备上传配置模型
class UploadConfig(BaseModel):
    telemetry_interval_ms: int
    event_immediate: bool

# 设备配置请求和响应模型
class DeviceConfigRequest(BaseModel):
    burst_pull: BurstPullConfig
    distance: DistanceConfig
    night_mode: NightModeConfig
    upload: UploadConfig

# 设备配置响应模型
class DeviceConfigResponse(BaseModel):
    saved: bool = True
    config_version: int

# 设备命令确认模型
class CommandAck(BaseModel):
    cmd_id: str
    accepted: bool
    node: str
    code: str
    message: str
    ts_ms: int = Field(default_factory=utc_now_ms)

# 设备记录模型
class DeviceRecord(BaseModel):
    pair_id: str
    handle_id: str
    collar_id: str
    handle_mac: Optional[str] = None
    collar_mac: Optional[str] = None
    protocol_version: int = 1
    created_at: int = Field(default_factory=utc_now_ms)
    hardware: HardwareInfo

# 设备遥测数据存储模型
class StoredTelemetry(TelemetryUpsertRequest):
    pass

# 设备事件存储模型
class StoredEvent(EventCreateRequest):
    pass

# 设备状态快照模型
class StatusSnapshot(BaseModel):
    pair_id: str
    online: bool
    last_seen_ms: Optional[int]
    handle: HandleStatus
    collar: CollarStatus
    active_alert: str

# 设备配置快照模型
class TopicEnvelope(BaseModel):
    topic: str
    payload: dict[str, Any]
