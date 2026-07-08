from __future__ import annotations

from fastapi import APIRouter

from app.models import TelemetryResponse, TelemetryUpsertRequest
from app.services.device_service import upsert_telemetry

router = APIRouter(tags=["telemetry"])


@router.post("/telemetry", response_model=TelemetryResponse)
def telemetry_endpoint(payload: TelemetryUpsertRequest) -> TelemetryResponse:
    upsert_telemetry(payload)
    return TelemetryResponse()
