from __future__ import annotations

from app.db.repository import repository
from app.models import (
    DeviceRegisterRequest,
    DeviceRegisterResponse,
    DeviceStatusResponse,
    DogProfile,
    DogProfileCreateRequest,
    DogProfileUpdateRequest,
    StoredTelemetry,
    TelemetryUpsertRequest,
)
from app.services.session_service import build_session_id

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

# 设备遥测数据上报服务模块
def upsert_telemetry(payload: TelemetryUpsertRequest) -> StoredTelemetry:
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
