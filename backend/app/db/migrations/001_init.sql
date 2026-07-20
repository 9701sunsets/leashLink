CREATE SEQUENCE IF NOT EXISTS leashlink_pair_seq START 1;

CREATE TABLE IF NOT EXISTS devices (
    pair_id TEXT PRIMARY KEY,
    handle_id TEXT UNIQUE,
    collar_id TEXT UNIQUE,
    handle_mac TEXT,
    collar_mac TEXT,
    protocol_version INTEGER NOT NULL DEFAULT 1,
    hardware JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS dog_profiles (
    id BIGSERIAL PRIMARY KEY,
    pair_id TEXT NOT NULL REFERENCES devices(pair_id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    owner TEXT NOT NULL DEFAULT '',
    breed TEXT NOT NULL DEFAULT 'mixed',
    age DOUBLE PRECISION NOT NULL DEFAULT 2,
    weight DOUBLE PRECISION NOT NULL DEFAULT 10,
    neutered BOOLEAN NOT NULL DEFAULT FALSE,
    calories_now INTEGER NOT NULL DEFAULT 0,
    walk_minutes INTEGER NOT NULL DEFAULT 0,
    distance_km DOUBLE PRECISION NOT NULL DEFAULT 0,
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS walk_sessions (
    session_id TEXT PRIMARY KEY,
    pair_id TEXT NOT NULL REFERENCES devices(pair_id) ON DELETE CASCADE,
    dog_id BIGINT REFERENCES dog_profiles(id) ON DELETE SET NULL,
    started_at_ms BIGINT NOT NULL,
    ended_at_ms BIGINT,
    steps INTEGER NOT NULL DEFAULT 0,
    duration_s INTEGER NOT NULL DEFAULT 0,
    burst_count INTEGER NOT NULL DEFAULT 0,
    distance_warning_count INTEGER NOT NULL DEFAULT 0,
    max_tension_n DOUBLE PRECISION NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS device_status_latest (
    pair_id TEXT PRIMARY KEY REFERENCES devices(pair_id) ON DELETE CASCADE,
    online BOOLEAN NOT NULL DEFAULT FALSE,
    last_seen_ms BIGINT,
    handle JSONB NOT NULL DEFAULT '{}'::jsonb,
    collar JSONB NOT NULL DEFAULT '{}'::jsonb,
    active_alert TEXT NOT NULL DEFAULT 'none'
);

CREATE TABLE IF NOT EXISTS telemetry_samples (
    id BIGSERIAL PRIMARY KEY,
    pair_id TEXT NOT NULL REFERENCES devices(pair_id) ON DELETE CASCADE,
    session_id TEXT NOT NULL,
    ts_ms BIGINT NOT NULL,
    handle JSONB NOT NULL DEFAULT '{}'::jsonb,
    collar JSONB NOT NULL DEFAULT '{}'::jsonb,
    location JSONB,
    alert TEXT NOT NULL DEFAULT 'none',
    raw_payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at BIGINT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_telemetry_pair_ts ON telemetry_samples(pair_id, ts_ms DESC);
CREATE INDEX IF NOT EXISTS idx_telemetry_session_ts ON telemetry_samples(session_id, ts_ms DESC);

CREATE TABLE IF NOT EXISTS safety_events (
    event_id TEXT PRIMARY KEY,
    pair_id TEXT NOT NULL REFERENCES devices(pair_id) ON DELETE CASCADE,
    session_id TEXT NOT NULL,
    type TEXT NOT NULL,
    severity TEXT NOT NULL,
    ts_ms BIGINT NOT NULL,
    source TEXT,
    metrics JSONB NOT NULL DEFAULT '{}'::jsonb,
    actions JSONB NOT NULL DEFAULT '[]'::jsonb,
    raw_payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at BIGINT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_events_pair_ts ON safety_events(pair_id, ts_ms DESC);
CREATE INDEX IF NOT EXISTS idx_events_session_ts ON safety_events(session_id, ts_ms DESC);
CREATE INDEX IF NOT EXISTS idx_events_pair_type_ts ON safety_events(pair_id, type, ts_ms DESC);

CREATE TABLE IF NOT EXISTS device_configs (
    pair_id TEXT PRIMARY KEY REFERENCES devices(pair_id) ON DELETE CASCADE,
    config_version INTEGER NOT NULL DEFAULT 0,
    fence JSONB,
    config JSONB,
    updated_at BIGINT NOT NULL
);

CREATE TABLE IF NOT EXISTS device_commands (
    cmd_id TEXT PRIMARY KEY,
    pair_id TEXT NOT NULL REFERENCES devices(pair_id) ON DELETE CASCADE,
    type TEXT NOT NULL,
    payload JSONB NOT NULL DEFAULT '{}'::jsonb,
    status TEXT NOT NULL DEFAULT 'pending',
    sent_at_ms BIGINT NOT NULL,
    ack_at_ms BIGINT
);

CREATE INDEX IF NOT EXISTS idx_commands_pair_sent ON device_commands(pair_id, sent_at_ms DESC);
