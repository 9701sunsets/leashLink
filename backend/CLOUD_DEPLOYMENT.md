# leashLink backend persistence and cloud deployment

This guide turns the backend from in-memory storage into PostgreSQL-backed persistence, then connects the device uplink to Alibaba Cloud IoT Platform.

## 1. Local PostgreSQL persistence

Start PostgreSQL:

```powershell
docker compose up -d postgres
```

Set the database URL before starting FastAPI:

```powershell
cd D:\ESP\leashLink\backend
$env:DATABASE_URL="postgresql://leashlink:leashlink123@127.0.0.1:5432/leashlink"
$env:AUTO_MIGRATE_DATABASE="1"
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

When `DATABASE_URL` is present, `app.db.repository` selects `PostgresRepository` and runs SQL files in `app/db/migrations`. If PostgreSQL is unavailable, `REPOSITORY_FALLBACK_TO_MEMORY=1` keeps local demos running with the in-memory repository.

Verify tables:

```powershell
docker exec -it leashlink-postgres psql -U leashlink -d leashlink -c "\dt"
```

Verify dog profile persistence:

```powershell
curl http://127.0.0.1:8000/api/v1/devices/LL-P-0001/dogs
```

## 2. Persisted data model

Core tables:

- `devices`: pair identity, handle/collar identity, protocol version, hardware info.
- `dog_profiles`: dog archive shown by the Web profile page.
- `device_status_latest`: latest merged state for fast dashboard reads.
- `telemetry_samples`: append-only telemetry snapshots.
- `safety_events`: append-only safety event timeline.
- `device_configs`: fence and runtime configuration, versioned by `config_version`.
- `walk_sessions`: session-level summary reserved for reports.
- `device_commands`: cloud command lifecycle reserved for IoT downlink ack tracking.

Recommended retention:

- Keep `safety_events`, `dog_profiles`, `device_configs`, and `walk_sessions` permanently.
- Keep `telemetry_samples` at 1 sample/second during demos.
- For production, archive or delete raw telemetry after 30 to 90 days.

## 3. Alibaba Cloud RDS PostgreSQL

Create an RDS PostgreSQL instance:

1. Region: choose the same region as the backend service and IoT Platform instance.
2. Engine: PostgreSQL.
3. Database: `leashlink`.
4. Account: create a normal application account, for example `leashlink_app`.
5. Network: place RDS and the backend ECS/ACK/VPC service in the same VPC.
6. Whitelist/security group: allow only the backend service network, not `0.0.0.0/0`.

Backend environment:

```text
DATABASE_URL=postgresql://leashlink_app:<password>@<rds-internal-endpoint>:5432/leashlink
AUTO_MIGRATE_DATABASE=1
REPOSITORY_FALLBACK_TO_MEMORY=0
```

For production, run migrations once in the release step, then set `AUTO_MIGRATE_DATABASE=0` for normal app startup.

## 4. Alibaba Cloud IoT Platform topology

Keep the local safety loop on ESP32-C3/ESP32-C5. Cloud only records, configures, alerts, and shows history.

```text
ESP32-C3 handle
  -> MQTT over TLS
  -> Alibaba Cloud IoT Platform
  -> AMQP server-side subscription
  -> backend ingestion worker
  -> PostgreSQL

Web / Mini Program
  -> HTTPS
  -> FastAPI
  -> PostgreSQL
  -> Alibaba Cloud IoT Pub API
  -> /user/cmd topic
  -> ESP32-C3 handle
  -> ESP-NOW
  -> ESP32-C5 collar
```

## 5. IoT product and topic plan

Create one IoT product for the handle gateway. Treat the handle as the cloud device because it owns Wi-Fi/MQTT and bridges the collar over ESP-NOW.

Identity mapping:

- `productKey`: Alibaba Cloud IoT product key.
- `deviceName`: use `pair_id` or `handle_id`. For demos, `LL-P-0001` is simplest.
- `deviceSecret`: one-device-one-secret stored in ESP32 NVS, never committed.

Custom topics:

```text
/${productKey}/${deviceName}/user/telemetry
/${productKey}/${deviceName}/user/event
/${productKey}/${deviceName}/user/cmd
/${productKey}/${deviceName}/user/cmd_ack
```

QoS:

- telemetry: QoS 0.
- event: QoS 1.
- cmd: QoS 1.
- cmd_ack: QoS 1.

Payloads should stay compatible with `通信协议.md`:

```json
{
  "protocol_version": 1,
  "pair_id": "LL-P-0001",
  "session_id": "20260706-001",
  "ts_ms": 1783334400123,
  "handle": {
    "tension_n": 8.4,
    "leash_locked": false,
    "battery_pct": 82
  },
  "collar": {
    "motion_state": "walk",
    "steps": 1523,
    "battery_pct": 76,
    "rssi_dbm": -58
  },
  "alert": "none"
}
```

## 6. Backend ingestion worker

Recommended path:

1. Enable server-side subscription for the IoT product.
2. Create a consumer group for the backend.
3. Subscribe to custom topic messages and device status messages.
4. Run a backend worker process that consumes AMQP messages.
5. Convert Alibaba topic/deviceName into `pair_id`.
6. For `/user/telemetry`, call the same service path as `POST /api/v1/telemetry`.
7. For `/user/event`, call the same service path as `POST /api/v1/events`.
8. Store raw payload in PostgreSQL for audit/debug, but only expose cleaned fields to the frontend.

Worker package layout:

```text
backend/app/iot/
  aliyun_amqp.py       # AMQP client and message loop
  aliyun_pub.py        # command publish helper
  normalizer.py        # IoT topic/payload -> app models
```

## 7. Command downlink

Frontend calls:

```text
POST /api/v1/devices/{pair_id}/commands
```

Backend should:

1. Insert row into `device_commands` with `status='pending'`.
2. Publish JSON to `/${productKey}/${deviceName}/user/cmd`.
3. Device executes locally or forwards to collar.
4. Device publishes `/user/cmd_ack`.
5. AMQP worker updates `device_commands.status`, `ack_at_ms`.

Example command:

```json
{
  "cmd_id": "cmd-1783334400123",
  "type": "find_dog",
  "ts_ms": 1783334400123,
  "payload": {
    "duration_ms": 10000
  }
}
```

## 8. Deployment order

1. Merge PostgreSQL repository and migrations.
2. Run locally with Docker PostgreSQL.
3. Confirm Web dog profiles survive backend restart.
4. Confirm telemetry/event API writes rows.
5. Create Alibaba Cloud RDS PostgreSQL.
6. Deploy backend to ECS, ACK, or another container service in the same VPC.
7. Set `DATABASE_URL` to the RDS internal endpoint.
8. Create Alibaba Cloud IoT product and one test device.
9. Flash ESP32-C3 with IoT MQTT credentials and custom topics.
10. Enable AMQP server-side subscription.
11. Add and run `iot_ingest_worker`.
12. Add IoT Pub helper for command downlink.
13. Add monitoring: backend health check, AMQP reconnect logs, database slow query logs.

## 9. Safety rules

- Do not move burst-pull decision making to cloud.
- Do not upload raw IMU/HX711 streams by default.
- Make all event writes idempotent by `event_id`.
- Keep `cmd_id` unique and require device ack for commands that affect feedback or configuration.
- Store device secrets only in Alibaba Cloud IoT and ESP32 NVS.
