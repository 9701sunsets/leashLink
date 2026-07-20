from __future__ import annotations

import os
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator, Optional


class PostgresUnavailable(RuntimeError):
    pass


class PostgresStore:
    def __init__(self, dsn: Optional[str] = None) -> None:
        self.dsn = dsn or os.getenv("DATABASE_URL")
        if not self.dsn:
            raise PostgresUnavailable("DATABASE_URL is not configured")
        try:
            import psycopg  # type: ignore
        except Exception as exc:  # pragma: no cover - optional dependency
            raise PostgresUnavailable("psycopg is not installed") from exc
        self._psycopg = psycopg

    # 上下文管理器，用于获取数据库连接
    @contextmanager
    def connection(self) -> Iterator[object]:
        with self._psycopg.connect(self.dsn) as conn:
            yield conn

    def migrate(self) -> None:
        migrations_dir = Path(__file__).with_name("migrations")
        with self.connection() as conn:
            with conn.cursor() as cur:
                for migration in sorted(migrations_dir.glob("*.sql")):
                    cur.execute(migration.read_text(encoding="utf-8"))
            conn.commit()
