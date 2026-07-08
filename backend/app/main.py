from __future__ import annotations

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from app.api.config import router as config_router
from app.api.device import router as device_router
from app.api.events import router as events_router
from app.api.telemetry import router as telemetry_router
from app.mqtt.publisher import publisher
from app.mqtt.subscriber import subscriber


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

    @app.on_event("startup")
    def startup() -> None:
        publisher.connect()
        subscriber.connect()

    @app.on_event("shutdown")
    def shutdown() -> None:
        publisher.disconnect()
        subscriber.disconnect()

    @app.get("/health")
    def health() -> dict[str, str]:
        return {"status": "ok"}

    return app


app = create_app()
