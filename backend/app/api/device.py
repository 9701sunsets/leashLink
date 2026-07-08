from __future__ import annotations

from fastapi import APIRouter, HTTPException

from app.models import DeviceRegisterRequest, DeviceRegisterResponse, DeviceStatusResponse, FenceConfigRequest
from app.services.config_service import set_fence
from app.services.device_service import get_status, register_device

router = APIRouter(prefix="/devices", tags=["devices"])


@router.post("/register", response_model=DeviceRegisterResponse)
def register_device_endpoint(payload: DeviceRegisterRequest) -> DeviceRegisterResponse:
    return register_device(payload)


@router.get("/{pair_id}/status", response_model=DeviceStatusResponse)
def device_status_endpoint(pair_id: str) -> DeviceStatusResponse:
    try:
        return get_status(pair_id)
    except KeyError as exc:
        raise HTTPException(status_code=404, detail="device not found") from exc


@router.put("/{pair_id}/fence")
def fence_endpoint(pair_id: str, payload: FenceConfigRequest):
    return set_fence(pair_id, payload)
