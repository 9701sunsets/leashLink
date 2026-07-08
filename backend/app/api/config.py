from __future__ import annotations

from fastapi import APIRouter

from app.models import DeviceConfigRequest, DeviceConfigResponse
from app.services.config_service import set_device_config

router = APIRouter(prefix="/devices", tags=["config"])


@router.put("/{pair_id}/config", response_model=DeviceConfigResponse)
def config_endpoint(pair_id: str, payload: DeviceConfigRequest) -> DeviceConfigResponse:
    return set_device_config(pair_id, payload)
