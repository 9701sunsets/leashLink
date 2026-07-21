from __future__ import annotations

from typing import Optional

from app.db.postgres import PostgresStore
from app.models import (
    CollarStatus,
    DeviceConfigRequest,
    DeviceRecord,
    DeviceStatusResponse,
    DogProfile,
    DogProfileCreateRequest,
    DogProfileUpdateRequest,
    EventListResponse,
    EventSummary,
    FenceConfigRequest,
    HandleStatus,
    HardwareInfo,
    StoredEvent,
    StoredTelemetry,
    utc_now_ms,
)


class PostgresRepository:
    def __init__(self, store: PostgresStore) -> None:
        self.store = store
        from psycopg.rows import dict_row  # type: ignore
        from psycopg.types.json import Jsonb  # type: ignore

        self._dict_row = dict_row
        self._jsonb = Jsonb

    def _json(self, value) -> object:
        return self._jsonb(value)

    def _ensure_device(self, pair_id: str) -> None:
        now = utc_now_ms()
        with self.store.connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO devices (pair_id, created_at, updated_at)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (pair_id) DO NOTHING
                    """,
                    (pair_id, now, now),
                )
            conn.commit()

    def _default_hardware(self) -> HardwareInfo:
        return HardwareInfo(handle_mcu="unknown", collar_mcu="unknown", handle_fw="unknown", collar_fw="unknown")

    def _row_to_device(self, row: dict) -> DeviceRecord:
        hardware = row.get("hardware") or {}
        return DeviceRecord(
            pair_id=row["pair_id"],
            handle_id=row.get("handle_id") or "",
            collar_id=row.get("collar_id") or "",
            handle_mac=row.get("handle_mac"),
            collar_mac=row.get("collar_mac"),
            protocol_version=row.get("protocol_version") or 1,
            created_at=row.get("created_at") or utc_now_ms(),
            hardware=HardwareInfo.model_validate(hardware) if hardware else self._default_hardware(),
        )

    def register_device(self, handle_id: str, collar_id: str, hardware: HardwareInfo) -> DeviceRecord:
        now = utc_now_ms()
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(
                    """
                    SELECT * FROM devices
                    WHERE handle_id = %s OR collar_id = %s
                    LIMIT 1
                    """,
                    (handle_id, collar_id),
                )
                existing = cur.fetchone()
                if existing:
                    return self._row_to_device(existing)

                cur.execute("SELECT nextval('leashlink_pair_seq') AS seq")
                seq = int(cur.fetchone()["seq"])
                pair_id = f"LL-P-{seq:04d}"
                cur.execute(
                    """
                    INSERT INTO devices (
                        pair_id, handle_id, collar_id, protocol_version, hardware, created_at, updated_at
                    )
                    VALUES (%s, %s, %s, %s, %s, %s, %s)
                    RETURNING *
                    """,
                    (pair_id, handle_id, collar_id, 1, self._json(hardware.model_dump(mode="json")), now, now),
                )
                row = cur.fetchone()
            conn.commit()
        return self._row_to_device(row)

    def get_device(self, pair_id: str) -> Optional[DeviceRecord]:
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute("SELECT * FROM devices WHERE pair_id = %s", (pair_id,))
                row = cur.fetchone()
        return self._row_to_device(row) if row else None

    def list_devices(self) -> list[DeviceRecord]:
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute("SELECT * FROM devices ORDER BY created_at DESC")
                rows = cur.fetchall()
        return [self._row_to_device(row) for row in rows]

    def _merge_status(self, current: Optional[DeviceStatusResponse], telemetry: StoredTelemetry) -> DeviceStatusResponse:
        status = current or DeviceStatusResponse(
            pair_id=telemetry.pair_id,
            online=False,
            last_seen_ms=None,
            handle=HandleStatus(),
            collar=CollarStatus(),
            active_alert="none",
        )
        status.online = True
        status.last_seen_ms = telemetry.ts_ms
        status.active_alert = telemetry.alert

        handle = status.handle.model_dump(mode="json")
        handle.update(telemetry.handle.model_dump(mode="json", exclude_none=True))
        collar = status.collar.model_dump(mode="json")
        collar.update(telemetry.collar.model_dump(mode="json", exclude_none=True))
        status.handle = HandleStatus.model_validate(handle)
        status.collar = CollarStatus.model_validate(collar)
        return status

    def upsert_telemetry(self, telemetry: StoredTelemetry) -> StoredTelemetry:
        self._ensure_device(telemetry.pair_id)
        status = self._merge_status(self.get_status(telemetry.pair_id), telemetry)
        now = utc_now_ms()
        raw_payload = telemetry.model_dump(mode="json")
        with self.store.connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO telemetry_samples (
                        pair_id, session_id, ts_ms, handle, collar, location, alert, raw_payload, created_at
                    )
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
                    """,
                    (
                        telemetry.pair_id,
                        telemetry.session_id,
                        telemetry.ts_ms,
                        self._json(telemetry.handle.model_dump(mode="json", exclude_none=True)),
                        self._json(telemetry.collar.model_dump(mode="json", exclude_none=True)),
                        self._json(telemetry.location.model_dump(mode="json")) if telemetry.location else None,
                        telemetry.alert,
                        self._json(raw_payload),
                        now,
                    ),
                )
                cur.execute(
                    """
                    INSERT INTO device_status_latest (pair_id, online, last_seen_ms, handle, collar, active_alert)
                    VALUES (%s, %s, %s, %s, %s, %s)
                    ON CONFLICT (pair_id) DO UPDATE SET
                        online = EXCLUDED.online,
                        last_seen_ms = EXCLUDED.last_seen_ms,
                        handle = EXCLUDED.handle,
                        collar = EXCLUDED.collar,
                        active_alert = EXCLUDED.active_alert
                    """,
                    (
                        status.pair_id,
                        status.online,
                        status.last_seen_ms,
                        self._json(status.handle.model_dump(mode="json")),
                        self._json(status.collar.model_dump(mode="json")),
                        status.active_alert,
                    ),
                )
            conn.commit()
        return telemetry

    def _bootstrap_default_dog(self, pair_id: str) -> None:
        now = utc_now_ms()
        self._ensure_device(pair_id)
        with self.store.connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO dog_profiles (
                        pair_id, name, owner, breed, age, weight, neutered,
                        calories_now, walk_minutes, distance_km, created_at, updated_at
                    )
                    SELECT %s, 'Luna', '小林', 'golden', 3, 30, false, 128, 45, 3.2, %s, %s
                    WHERE NOT EXISTS (SELECT 1 FROM dog_profiles WHERE pair_id = %s)
                    """,
                    (pair_id, now, now, pair_id),
                )
            conn.commit()

    def _row_to_dog(self, row: dict) -> DogProfile:
        return DogProfile.model_validate(dict(row))

    def list_dog_profiles(self, pair_id: str) -> list[DogProfile]:
        self._bootstrap_default_dog(pair_id)
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute("SELECT * FROM dog_profiles WHERE pair_id = %s ORDER BY id", (pair_id,))
                rows = cur.fetchall()
        return [self._row_to_dog(row) for row in rows]

    def create_dog_profile(self, pair_id: str, payload: DogProfileCreateRequest) -> DogProfile:
        self._ensure_device(pair_id)
        now = utc_now_ms()
        data = payload.model_dump()
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(
                    """
                    INSERT INTO dog_profiles (
                        pair_id, name, owner, breed, age, weight, neutered,
                        calories_now, walk_minutes, distance_km, created_at, updated_at
                    )
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                    RETURNING *
                    """,
                    (
                        pair_id,
                        data["name"],
                        data["owner"],
                        data["breed"],
                        data["age"],
                        data["weight"],
                        data["neutered"],
                        data["calories_now"],
                        data["walk_minutes"],
                        data["distance_km"],
                        now,
                        now,
                    ),
                )
                row = cur.fetchone()
            conn.commit()
        return self._row_to_dog(row)

    def update_dog_profile(self, pair_id: str, dog_id: int, payload: DogProfileUpdateRequest) -> DogProfile:
        updates = payload.model_dump(exclude_unset=True)
        if not updates:
            with self.store.connection() as conn:
                with conn.cursor(row_factory=self._dict_row) as cur:
                    cur.execute("SELECT * FROM dog_profiles WHERE pair_id = %s AND id = %s", (pair_id, dog_id))
                    row = cur.fetchone()
            if not row:
                raise KeyError(dog_id)
            return self._row_to_dog(row)

        allowed = {
            "name",
            "owner",
            "breed",
            "age",
            "weight",
            "neutered",
            "calories_now",
            "walk_minutes",
            "distance_km",
        }
        assignments = [f"{key} = %s" for key in updates if key in allowed]
        values = [updates[key] for key in updates if key in allowed]
        values.extend([utc_now_ms(), pair_id, dog_id])
        sql = f"""
            UPDATE dog_profiles
            SET {", ".join(assignments)}, updated_at = %s
            WHERE pair_id = %s AND id = %s
            RETURNING *
        """
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(sql, values)
                row = cur.fetchone()
            conn.commit()
        if not row:
            raise KeyError(dog_id)
        return self._row_to_dog(row)

    def delete_dog_profile(self, pair_id: str, dog_id: int) -> None:
        with self.store.connection() as conn:
            with conn.cursor() as cur:
                cur.execute("DELETE FROM dog_profiles WHERE pair_id = %s AND id = %s", (pair_id, dog_id))
                deleted = cur.rowcount
            conn.commit()
        if not deleted:
            raise KeyError(dog_id)

    def get_status(self, pair_id: str) -> Optional[DeviceStatusResponse]:
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute("SELECT * FROM device_status_latest WHERE pair_id = %s", (pair_id,))
                row = cur.fetchone()
        if not row:
            return None
        return DeviceStatusResponse(
            pair_id=row["pair_id"],
            online=row["online"],
            last_seen_ms=row["last_seen_ms"],
            handle=HandleStatus.model_validate(row["handle"] or {}),
            collar=CollarStatus.model_validate(row["collar"] or {}),
            active_alert=row["active_alert"] or "none",
        )

    def add_event(self, event: StoredEvent) -> StoredEvent:
        self._ensure_device(event.pair_id)
        now = utc_now_ms()
        with self.store.connection() as conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO safety_events (
                        event_id, pair_id, session_id, type, severity, ts_ms, source,
                        metrics, actions, raw_payload, created_at
                    )
                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                    ON CONFLICT (event_id) DO UPDATE SET
                        severity = EXCLUDED.severity,
                        metrics = EXCLUDED.metrics,
                        actions = EXCLUDED.actions
                    """,
                    (
                        event.event_id,
                        event.pair_id,
                        event.session_id,
                        event.type,
                        event.severity,
                        event.ts_ms,
                        event.source,
                        self._json(event.metrics.model_dump(mode="json", exclude_none=True)),
                        self._json(event.actions),
                        self._json(event.model_dump(mode="json")),
                        now,
                    ),
                )
                cur.execute(
                    """
                    INSERT INTO device_status_latest (pair_id, online, last_seen_ms, handle, collar, active_alert)
                    VALUES (%s, true, %s, '{}'::jsonb, '{}'::jsonb, %s)
                    ON CONFLICT (pair_id) DO UPDATE SET
                        last_seen_ms = EXCLUDED.last_seen_ms,
                        active_alert = EXCLUDED.active_alert
                    """,
                    (event.pair_id, event.ts_ms, event.severity),
                )
            conn.commit()
        return event

    def list_events(
        self,
        pair_id: str,
        session_id: Optional[str] = None,
        event_type: Optional[str] = None,
    ) -> EventListResponse:
        conditions = ["pair_id = %s"]
        values: list[object] = [pair_id]
        if session_id is not None:
            conditions.append("session_id = %s")
            values.append(session_id)
        if event_type is not None:
            conditions.append("type = %s")
            values.append(event_type)

        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(
                    f"""
                    SELECT event_id, type, severity, ts_ms, metrics
                    FROM safety_events
                    WHERE {" AND ".join(conditions)}
                    ORDER BY ts_ms DESC
                    """,
                    values,
                )
                rows = cur.fetchall()

        items = [
            EventSummary(
                event_id=row["event_id"],
                type=row["type"],
                severity=row["severity"],
                ts_ms=row["ts_ms"],
                summary=self._summarize_event(row["type"], row["severity"], row.get("metrics") or {}),
            )
            for row in rows
        ]
        return EventListResponse(items=items, next_cursor=None)

    def _summarize_event(self, event_type: str, severity: str, metrics: dict) -> str:
        if event_type == "burst_pull":
            tension = metrics.get("tension_peak_n") or 0
            latency = metrics.get("response_latency_ms") or 0
            return f"张力峰值 {float(tension):.1f}N，系统 {latency}ms 内完成处理"
        if event_type == "link_lost":
            return "通信链路中断"
        if event_type == "distance_warning":
            distance = metrics.get("distance_est_m") or 0
            return f"距离告警，估计距离 {float(distance):.1f}m"
        return f"{event_type} / {severity}"

    def upsert_fence(self, pair_id: str, fence: FenceConfigRequest) -> int:
        self._ensure_device(pair_id)
        now = utc_now_ms()
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(
                    """
                    INSERT INTO device_configs (pair_id, config_version, fence, updated_at)
                    VALUES (%s, 1, %s, %s)
                    ON CONFLICT (pair_id) DO UPDATE SET
                        config_version = device_configs.config_version + 1,
                        fence = EXCLUDED.fence,
                        updated_at = EXCLUDED.updated_at
                    RETURNING config_version
                    """,
                    (pair_id, self._json(fence.model_dump(mode="json")), now),
                )
                version = int(cur.fetchone()["config_version"])
            conn.commit()
        return version

    def upsert_config(self, pair_id: str, config: DeviceConfigRequest) -> int:
        self._ensure_device(pair_id)
        now = utc_now_ms()
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute(
                    """
                    INSERT INTO device_configs (pair_id, config_version, config, updated_at)
                    VALUES (%s, 1, %s, %s)
                    ON CONFLICT (pair_id) DO UPDATE SET
                        config_version = device_configs.config_version + 1,
                        config = EXCLUDED.config,
                        updated_at = EXCLUDED.updated_at
                    RETURNING config_version
                    """,
                    (pair_id, self._json(config.model_dump(mode="json")), now),
                )
                version = int(cur.fetchone()["config_version"])
            conn.commit()
        return version

    def get_config(self, pair_id: str) -> Optional[DeviceConfigRequest]:
        with self.store.connection() as conn:
            with conn.cursor(row_factory=self._dict_row) as cur:
                cur.execute("SELECT config FROM device_configs WHERE pair_id = %s", (pair_id,))
                row = cur.fetchone()
        if not row or not row.get("config"):
            return None
        return DeviceConfigRequest.model_validate(row["config"])
