from __future__ import annotations

from fastapi import APIRouter

from app.models import TelemetryResponse, TelemetryUpsertRequest
from app.services.device_service import upsert_telemetry

# 设备遥测数据上报路由模块
router = APIRouter(tags=["telemetry"])

# 设备遥测数据上报接口
@router.post("/telemetry", response_model=TelemetryResponse)
def telemetry_endpoint(payload: TelemetryUpsertRequest) -> TelemetryResponse:
    upsert_telemetry(payload)
    return TelemetryResponse()
