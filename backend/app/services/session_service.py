from __future__ import annotations

from datetime import datetime, timezone


def build_session_id() -> str:
    return datetime.now(tz=timezone.utc).strftime("%Y%m%d-%H%M%S")
