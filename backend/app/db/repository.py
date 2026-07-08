from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from threading import RLock
from typing import Optional

from app.models import (
    CollarStatus,
    DeviceConfigRequest,
    DeviceRecord,
    DeviceStatusResponse,
    EventListResponse,
    EventSummary,
    FenceConfigRequest,
    HandleStatus,
    StoredEvent,
    StoredTelemetry,
)


@dataclass
class DeviceRuntimeState:
    status: DeviceStatusResponse
    telemetry: Optional[StoredTelemetry] = None
    fence: Optional[FenceConfigRequest] = None
    config: Optional[DeviceConfigRequest] = None
    config_version: int = 0


class InMemoryRepository:
    def __init__(self) -> None:
        self._lock = RLock()
        self._sequence = 0
        self._devices: dict[str, DeviceRecord] = {}
        self._runtime: dict[str, DeviceRuntimeState] = {}
        self._events_by_pair: dict[str, list[StoredEvent]] = defaultdict(list)
        self._sessions_by_pair: dict[str, set[str]] = defaultdict(set)

    def _next_pair_id(self) -> str:
        self._sequence += 1
        return f"LL-P-{self._sequence:04d}"

    def register_device(self, handle_id: str, collar_id: str, hardware) -> DeviceRecord:
        with self._lock:
            for device in self._devices.values():
                if device.handle_id == handle_id or device.collar_id == collar_id:
                    return device

            pair_id = self._next_pair_id()
            record = DeviceRecord(
                pair_id=pair_id,
                handle_id=handle_id,
                collar_id=collar_id,
                hardware=hardware,
            )
            self._devices[pair_id] = record
            self._runtime[pair_id] = DeviceRuntimeState(
                status=DeviceStatusResponse(
                    pair_id=pair_id,
                    online=False,
                    last_seen_ms=None,
                    handle=HandleStatus(),
                    collar=CollarStatus(),
                    active_alert="none",
                )
            )
            return record

    def get_device(self, pair_id: str) -> Optional[DeviceRecord]:
        return self._devices.get(pair_id)

    def list_devices(self) -> list[DeviceRecord]:
        return list(self._devices.values())

    def upsert_telemetry(self, telemetry: StoredTelemetry) -> StoredTelemetry:
        with self._lock:
            runtime = self._runtime.setdefault(
                telemetry.pair_id,
                DeviceRuntimeState(
                    status=DeviceStatusResponse(
                        pair_id=telemetry.pair_id,
                        online=False,
                        last_seen_ms=None,
                        handle=HandleStatus(),
                        collar=CollarStatus(),
                        active_alert="none",
                    )
                ),
            )
            runtime.telemetry = telemetry
            runtime.status.online = True
            runtime.status.last_seen_ms = telemetry.ts_ms
            runtime.status.active_alert = telemetry.alert
            if telemetry.handle.battery_pct is not None:
                runtime.status.handle.battery_pct = telemetry.handle.battery_pct
            if telemetry.handle.tension_n is not None:
                runtime.status.handle.tension_n = telemetry.handle.tension_n
            if telemetry.handle.leash_locked is not None:
                runtime.status.handle.leash_locked = telemetry.handle.leash_locked
            if telemetry.handle.gps is not None:
                runtime.status.handle.gps = telemetry.handle.gps
            if telemetry.collar.battery_pct is not None:
                runtime.status.collar.battery_pct = telemetry.collar.battery_pct
            if telemetry.collar.motion_state is not None:
                runtime.status.collar.motion_state = telemetry.collar.motion_state
            if telemetry.collar.steps is not None:
                runtime.status.collar.steps = telemetry.collar.steps
            if telemetry.collar.rssi_dbm is not None:
                runtime.status.collar.rssi_dbm = telemetry.collar.rssi_dbm
            if telemetry.collar.distance_est_m is not None:
                runtime.status.collar.distance_est_m = telemetry.collar.distance_est_m
            return telemetry

    def get_status(self, pair_id: str) -> Optional[DeviceStatusResponse]:
        runtime = self._runtime.get(pair_id)
        if runtime is None:
            return None
        return runtime.status

    def add_event(self, event: StoredEvent) -> StoredEvent:
        with self._lock:
            self._events_by_pair[event.pair_id].append(event)
            self._sessions_by_pair[event.pair_id].add(event.session_id)
            runtime = self._runtime.setdefault(
                event.pair_id,
                DeviceRuntimeState(
                    status=DeviceStatusResponse(
                        pair_id=event.pair_id,
                        online=False,
                        last_seen_ms=None,
                        handle=HandleStatus(),
                        collar=CollarStatus(),
                        active_alert="none",
                    )
                ),
            )
            runtime.status.active_alert = event.severity
            runtime.status.last_seen_ms = event.ts_ms
            return event

    def list_events(
        self,
        pair_id: str,
        session_id: Optional[str] = None,
        event_type: Optional[str] = None,
    ) -> EventListResponse:
        events = self._events_by_pair.get(pair_id, [])
        if session_id is not None:
            events = [event for event in events if event.session_id == session_id]
        if event_type is not None:
            events = [event for event in events if event.type == event_type]
        items = [
            EventSummary(
                event_id=event.event_id,
                type=event.type,
                severity=event.severity,
                ts_ms=event.ts_ms,
                summary=self._summarize_event(event),
            )
            for event in events
        ]
        return EventListResponse(items=items, next_cursor=None)

    def _summarize_event(self, event: StoredEvent) -> str:
        if event.type == "burst_pull":
            tension = event.metrics.tension_peak_n if event.metrics.tension_peak_n is not None else 0
            latency = event.metrics.response_latency_ms if event.metrics.response_latency_ms is not None else 0
            return f"张力峰值 {tension:.1f}N，系统 {latency}ms 内完成处理"
        if event.type == "link_lost":
            return "通信链路中断"
        if event.type == "distance_warning":
            distance = event.metrics.distance_est_m if event.metrics.distance_est_m is not None else 0
            return f"距离告警，估计距离 {distance:.1f}m"
        return f"{event.type} / {event.severity}"

    def upsert_fence(self, pair_id: str, fence: FenceConfigRequest) -> int:
        with self._lock:
            runtime = self._runtime.setdefault(
                pair_id,
                DeviceRuntimeState(
                    status=DeviceStatusResponse(
                        pair_id=pair_id,
                        online=False,
                        last_seen_ms=None,
                        handle=HandleStatus(),
                        collar=CollarStatus(),
                        active_alert="none",
                    )
                ),
            )
            runtime.fence = fence
            runtime.config_version += 1
            return runtime.config_version

    def upsert_config(self, pair_id: str, config: DeviceConfigRequest) -> int:
        with self._lock:
            runtime = self._runtime.setdefault(
                pair_id,
                DeviceRuntimeState(
                    status=DeviceStatusResponse(
                        pair_id=pair_id,
                        online=False,
                        last_seen_ms=None,
                        handle=HandleStatus(),
                        collar=CollarStatus(),
                        active_alert="none",
                    )
                ),
            )
            runtime.config = config
            runtime.config_version += 1
            return runtime.config_version


repository = InMemoryRepository()
