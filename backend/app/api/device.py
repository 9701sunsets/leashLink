from __future__ import annotations

from fastapi import APIRouter, HTTPException

from app.models import DeviceRegisterRequest, DeviceRegisterResponse, DeviceStatusResponse, FenceConfigRequest
from app.mqtt.publisher import publisher
from app.models import utc_now_ms
from app.services.config_service import set_fence
from app.services.device_service import get_status, register_device

# 设备路由模块
router = APIRouter(prefix="/devices", tags=["devices"])

# 设备注册接口
@router.post("/register", response_model=DeviceRegisterResponse)
def register_device_endpoint(payload: DeviceRegisterRequest) -> DeviceRegisterResponse:
    return register_device(payload)

# 设备状态查询接口
@router.get("/{pair_id}/status", response_model=DeviceStatusResponse)
def device_status_endpoint(pair_id: str) -> DeviceStatusResponse:
    try:
        return get_status(pair_id)
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="device not found") from exc

# 设备围栏配置设置接口
@router.put("/{pair_id}/fence")
def fence_endpoint(pair_id: str, payload: FenceConfigRequest):
    return set_fence(pair_id, payload)


@router.post("/{pair_id}/commands")
def command_endpoint(pair_id: str, payload: dict):
    cmd_id = str(payload.get("cmd_id") or f"cmd-{utc_now_ms()}")
    command = {
        "cmd_id": cmd_id,
        "type": payload.get("type"),
        "ts_ms": utc_now_ms(),
        "payload": payload.get("payload") or {},
    }
    publisher.publish(f"leashlink/{pair_id}/cmd", command, qos=1)
    return {"accepted": True, "cmd_id": cmd_id}
