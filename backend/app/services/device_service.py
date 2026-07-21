from __future__ import annotations

from app.db.repository import repository
from app.models import (
    BurstPullConfig,
    DeviceRegisterRequest,
    DeviceRegisterResponse,
    DeviceStatusResponse,
    DeviceConfigRequest,
    DistanceConfig,
    DogProfile,
    DogProfileCreateRequest,
    DogProfileUpdateRequest,
    NightModeConfig,
    StoredTelemetry,
    TelemetryUpsertRequest,
    UploadConfig,
    WalkReportResponse,
)
from app.services.session_service import build_session_id

LIGHT_ADC_MAX_RAW = 4095
LIGHT_RAW_TO_LUX_DIVISOR = 4.0


def _lux_from_inverse_raw(raw: int) -> float:
    clamped_raw = max(0, min(LIGHT_ADC_MAX_RAW, int(raw)))
    return round((LIGHT_ADC_MAX_RAW - clamped_raw) / LIGHT_RAW_TO_LUX_DIVISOR, 2)


def _normalize_light(payload: TelemetryUpsertRequest) -> TelemetryUpsertRequest:
    if payload.handle.ambient_light_raw is None:
        return payload

    handle = payload.handle.model_dump()
    handle["ambient_light_lux"] = _lux_from_inverse_raw(payload.handle.ambient_light_raw)
    return payload.model_copy(update={"handle": payload.handle.__class__.model_validate(handle)})

# 设备注册服务模块
def register_device(payload: DeviceRegisterRequest) -> DeviceRegisterResponse:
    record = repository.register_device(payload.handle_id, payload.collar_id, payload.hardware)
    return DeviceRegisterResponse(
        pair_id=record.pair_id,
        mqtt_topic_prefix=f"leashlink/{record.pair_id}",
        created_at=record.created_at,
    )

# 设备状态查询服务模块
def get_status(pair_id: str) -> DeviceStatusResponse:
    status = repository.get_status(pair_id)
    if status is None:
        raise KeyError(pair_id)
    return status


def _default_device_config() -> DeviceConfigRequest:
    return DeviceConfigRequest(
        burst_pull=BurstPullConfig(tension_warn_n=12, tension_lock_n=20, accel_peak_g=1.8, hold_ms=300),
        distance=DistanceConfig(warn_m=10, danger_m=20),
        night_mode=NightModeConfig(enabled=True, light_threshold_lux=40),
        upload=UploadConfig(telemetry_interval_ms=1000, event_immediate=True),
    )


def get_device_config(pair_id: str) -> DeviceConfigRequest:
    get_config = getattr(repository, "get_config", None)
    if callable(get_config):
        config = get_config(pair_id)
        if config is not None:
            return config
    return _default_device_config()


def _build_tension_series(current: float, peak: float) -> list[float]:
    base = max(0.0, float(current or 0.0))
    high = max(base, float(peak or 0.0))
    if high <= 0:
        return [0.0] * 12
    if abs(high - base) < 0.01:
        return [round(base, 1)] * 12

    factors = [0, 0.12, 0.04, 0.22, 0.35, 0.18, 0.5, 1.0, 0.42, 0.25, 0.1, 0.05]
    return [round(base + (high - base) * factor, 1) for factor in factors]


def get_today_report(pair_id: str) -> WalkReportResponse:
    status = repository.get_status(pair_id)
    if status is None:
        return WalkReportResponse(session_id=f"{pair_id}-today", tension_series=[0.0] * 12)

    steps = int(status.collar.steps or 0)
    current_tension = float(status.handle.tension_n or 0.0)
    peak_tension = float(status.handle.tension_peak_n or current_tension)
    distance_from_steps_km = steps * 0.0007
    distance_from_link_km = float(status.collar.distance_est_m or 0.0) / 1000.0
    distance_km = round(max(distance_from_steps_km, distance_from_link_km), 2)
    duration_min = int(round(steps / 90)) if steps > 0 else 0

    try:
        burst_count = len(repository.list_events(pair_id=pair_id, event_type="burst_pull").items)
    except Exception:
        burst_count = 0

    health_state = "healthy"
    if status.active_alert != "none" or peak_tension >= 20:
        health_state = "attention"

    return WalkReportResponse(
        session_id=f"{pair_id}-today",
        duration_min=duration_min,
        distance_km=distance_km,
        steps=steps,
        max_tension_n=round(peak_tension, 1),
        burst_count=burst_count,
        health_state=health_state,
        tension_series=_build_tension_series(current_tension, peak_tension),
    )

# 设备遥测数据上报服务模块
def upsert_telemetry(payload: TelemetryUpsertRequest) -> StoredTelemetry:
    payload = _normalize_light(payload)
    telemetry = StoredTelemetry.model_validate(payload.model_dump())
    repository.upsert_telemetry(telemetry)
    return telemetry

# 设备会话ID确保服务模块
def ensure_session_id(session_id: str | None = None) -> str:
    return session_id or build_session_id()


def list_dog_profiles(pair_id: str) -> list[DogProfile]:
    return repository.list_dog_profiles(pair_id)


def create_dog_profile(pair_id: str, payload: DogProfileCreateRequest) -> DogProfile:
    return repository.create_dog_profile(pair_id, payload)


def update_dog_profile(pair_id: str, dog_id: int, payload: DogProfileUpdateRequest) -> DogProfile:
    return repository.update_dog_profile(pair_id, dog_id, payload)


def delete_dog_profile(pair_id: str, dog_id: int) -> None:
    repository.delete_dog_profile(pair_id, dog_id)
