from __future__ import annotations

import logging
import os
import time
from typing import Any

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from starlette.requests import Request

from app.api.config import router as config_router
from app.api.device import router as device_router
from app.api.events import router as events_router
from app.api.telemetry import router as telemetry_router
from app.models import EventCreateRequest, EventMetrics, TelemetryUpsertRequest, utc_now_ms
from app.mqtt.publisher import publisher
from app.mqtt.subscriber import subscriber
from app.services.device_service import upsert_telemetry
from app.services.event_service import create_event

logger = logging.getLogger("uvicorn.error")

MOTION_STATES = {
    0: "idle",
    1: "walk",
    2: "run",
    3: "burst",
    4: "shake",
    255: "unknown",
}

EVENT_TYPES = {
    1: "burst_pull",
    2: "distance_warning",
    3: "fence_breach",
    4: "fatigue_detected",
    5: "low_battery",
    6: "link_lost",
}

SEVERITIES = {
    0: "safe",
    1: "watch",
    2: "warning",
    3: "alert",
    4: "critical",
}


def _topic_pair_id(topic: str) -> str | None:
    parts = topic.split("/")
    if len(parts) >= 3 and parts[0] == "leashlink":
        return parts[1]
    return None


def _normalize_motion_state(value: Any) -> str:
    if isinstance(value, str):
        return value
    return MOTION_STATES.get(int(value), "unknown") if isinstance(value, (int, float)) else "unknown"


def handle_mqtt_message(topic: str, payload: dict[str, Any]) -> None:
    pair_id = str(payload.get("pair_id") or _topic_pair_id(topic) or "")
    if not pair_id:
        logger.info("MQTT message ignored topic=%s reason=missing pair_id payload=%s", topic, payload)
        return

    if topic.endswith("/telemetry"):
        telemetry_payload = dict(payload)
        telemetry_payload["pair_id"] = pair_id
        telemetry_payload.setdefault("session_id", "mqtt-session")
        telemetry_payload.setdefault("ts_ms", utc_now_ms())
        telemetry_payload.setdefault("handle", {})
        telemetry_payload.setdefault("collar", {})
        telemetry_payload.setdefault("alert", "none")

        collar = dict(telemetry_payload["collar"] or {})
        collar["motion_state"] = _normalize_motion_state(collar.get("motion_state"))
        telemetry_payload["collar"] = collar

        telemetry = TelemetryUpsertRequest.model_validate(telemetry_payload)
        upsert_telemetry(telemetry)
        logger.info(
            "Stored telemetry pair_id=%s session_id=%s tension=%.2fN peak=%.2fN locked=%s light=%s lux gps_fix=%s collar_motion=%s steps=%s accel=%s rssi=%s distance=%s",
            telemetry.pair_id,
            telemetry.session_id,
            telemetry.handle.tension_n or 0.0,
            telemetry.handle.tension_peak_n or 0.0,
            telemetry.handle.leash_locked,
            telemetry.handle.ambient_light_lux,
            telemetry.handle.gps.fix if telemetry.handle.gps else None,
            telemetry.collar.motion_state,
            telemetry.collar.steps,
            telemetry.collar.accel_peak_g,
            telemetry.collar.rssi_dbm,
            telemetry.collar.distance_est_m,
        )
        return

    if topic.endswith("/event"):
        event_type_raw = payload.get("type", "unknown")
        severity_raw = payload.get("severity", "unknown")
        event_type = EVENT_TYPES.get(event_type_raw, str(event_type_raw)) if isinstance(event_type_raw, int) else str(event_type_raw)
        severity = SEVERITIES.get(severity_raw, str(severity_raw)) if isinstance(severity_raw, int) else str(severity_raw)
        metrics = payload.get("metrics") or {}

        event = EventCreateRequest(
            pair_id=pair_id,
            session_id=str(payload.get("session_id") or "mqtt-session"),
            event_id=f"mqtt-{pair_id}-{payload.get('ts_ms') or utc_now_ms()}",
            type=event_type,
            severity=severity,
            ts_ms=int(payload.get("ts_ms") or utc_now_ms()),
            source="mqtt",
            metrics=EventMetrics(
                tension_peak_n=metrics.get("tension_peak_n"),
                accel_peak_g=metrics.get("accel_peak_g"),
                distance_est_m=metrics.get("distance_m") or metrics.get("distance_est_m"),
                response_latency_ms=metrics.get("latency_ms"),
            ),
        )
        create_event(event)
        logger.info(
            "Stored event pair_id=%s session_id=%s type=%s severity=%s metrics=%s",
            event.pair_id,
            event.session_id,
            event.type,
            event.severity,
            event.metrics.model_dump(exclude_none=True),
        )
        return

    if topic.endswith("/status") or topic.endswith("/cmd_ack"):
        logger.info("MQTT device message topic=%s payload=%s", topic, payload)
        return

    logger.info("MQTT message ignored topic=%s reason=unsupported topic payload=%s", topic, payload)


def create_app() -> FastAPI:
    app = FastAPI(title="leashLink Cloud API", version="0.1.0")
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    api_prefix = "/api/v1"
    app.include_router(device_router, prefix=api_prefix)
    app.include_router(events_router, prefix=api_prefix)
    app.include_router(telemetry_router, prefix=api_prefix)
    app.include_router(config_router, prefix=api_prefix)

    @app.middleware("http")
    async def log_http_requests(request: Request, call_next):
        started_at = time.perf_counter()
        client_host = request.client.host if request.client else "unknown"
        try:
            response = await call_next(request)
        except Exception:
            elapsed_ms = (time.perf_counter() - started_at) * 1000
            logger.exception(
                "HTTP %s %s failed client=%s elapsed=%.1fms",
                request.method,
                request.url.path,
                client_host,
                elapsed_ms,
            )
            raise

        elapsed_ms = (time.perf_counter() - started_at) * 1000
        logger.info(
            "HTTP %s %s status=%s client=%s elapsed=%.1fms",
            request.method,
            request.url.path,
            response.status_code,
            client_host,
            elapsed_ms,
        )
        return response

    @app.on_event("startup")
    def startup() -> None:
        mqtt_host = os.getenv("MQTT_HOST") or "not configured"
        mqtt_port = os.getenv("MQTT_PORT", "1883")
        logger.info("leashLink backend starting api_prefix=%s mqtt=%s:%s", api_prefix, mqtt_host, mqtt_port)
        subscriber.set_handler(handle_mqtt_message)
        publisher.connect()
        subscriber.connect()
        subscriber.subscribe("leashlink/+/telemetry", qos=0)
        subscriber.subscribe("leashlink/+/event", qos=1)
        subscriber.subscribe("leashlink/+/status", qos=1)
        subscriber.subscribe("leashlink/+/cmd_ack", qos=1)

    @app.on_event("shutdown")
    def shutdown() -> None:
        logger.info("leashLink backend shutting down")
        publisher.disconnect()
        subscriber.disconnect()

    @app.get("/health")
    def health() -> dict[str, str]:
        return {"status": "ok"}

    return app


app = create_app()
