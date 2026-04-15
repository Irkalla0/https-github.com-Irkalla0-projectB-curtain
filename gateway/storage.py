from __future__ import annotations

import json
import sqlite3
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class CurtainStatus:
    curtain_id: str
    state: str
    current_pos: int
    target_pos: int
    fault_code: int
    updated_at: int


class GatewayStorage:
    def __init__(self, db_path: str) -> None:
        self._db_path = Path(db_path)
        self._db_path.parent.mkdir(parents=True, exist_ok=True)
        self._init_db()

    def _connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self._db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def _init_db(self) -> None:
        with self._connect() as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS curtain_status (
                    curtain_id TEXT PRIMARY KEY,
                    state TEXT NOT NULL,
                    current_pos INTEGER NOT NULL,
                    target_pos INTEGER NOT NULL,
                    fault_code INTEGER NOT NULL,
                    updated_at INTEGER NOT NULL
                );

                CREATE TABLE IF NOT EXISTS fault_logs (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    curtain_id TEXT NOT NULL,
                    fault_code INTEGER NOT NULL,
                    detail TEXT NOT NULL,
                    ts INTEGER NOT NULL
                );

                CREATE TABLE IF NOT EXISTS audit_logs (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    actor TEXT NOT NULL,
                    action TEXT NOT NULL,
                    detail TEXT NOT NULL,
                    ts INTEGER NOT NULL
                );

                CREATE TABLE IF NOT EXISTS outbound_queue (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    curtain_id TEXT NOT NULL,
                    cmd TEXT NOT NULL,
                    payload TEXT NOT NULL,
                    ts INTEGER NOT NULL
                );
                """
            )

    def upsert_status(self, status: CurtainStatus) -> None:
        with self._connect() as conn:
            conn.execute(
                """
                INSERT INTO curtain_status(curtain_id, state, current_pos, target_pos, fault_code, updated_at)
                VALUES(?, ?, ?, ?, ?, ?)
                ON CONFLICT(curtain_id) DO UPDATE SET
                    state=excluded.state,
                    current_pos=excluded.current_pos,
                    target_pos=excluded.target_pos,
                    fault_code=excluded.fault_code,
                    updated_at=excluded.updated_at
                """,
                (
                    status.curtain_id,
                    status.state,
                    status.current_pos,
                    status.target_pos,
                    status.fault_code,
                    status.updated_at,
                ),
            )

    def get_status(self, curtain_id: str) -> CurtainStatus | None:
        with self._connect() as conn:
            row = conn.execute(
                "SELECT curtain_id, state, current_pos, target_pos, fault_code, updated_at FROM curtain_status WHERE curtain_id=?",
                (curtain_id,),
            ).fetchone()
        if row is None:
            return None
        return CurtainStatus(
            curtain_id=str(row["curtain_id"]),
            state=str(row["state"]),
            current_pos=int(row["current_pos"]),
            target_pos=int(row["target_pos"]),
            fault_code=int(row["fault_code"]),
            updated_at=int(row["updated_at"]),
        )

    def append_fault(self, curtain_id: str, fault_code: int, detail: str, ts: int) -> None:
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO fault_logs(curtain_id, fault_code, detail, ts) VALUES(?, ?, ?, ?)",
                (curtain_id, fault_code, detail, ts),
            )

    def list_faults(self, curtain_id: str, limit: int = 50) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                "SELECT id, curtain_id, fault_code, detail, ts FROM fault_logs WHERE curtain_id=? ORDER BY id DESC LIMIT ?",
                (curtain_id, limit),
            ).fetchall()
        return [dict(row) for row in rows]

    def append_audit(self, actor: str, action: str, detail: dict[str, Any], ts: int) -> None:
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO audit_logs(actor, action, detail, ts) VALUES(?, ?, ?, ?)",
                (actor, action, json.dumps(detail, ensure_ascii=False), ts),
            )

    def enqueue_outbound(self, curtain_id: str, cmd: str, payload: dict[str, Any], ts: int) -> None:
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO outbound_queue(curtain_id, cmd, payload, ts) VALUES(?, ?, ?, ?)",
                (curtain_id, cmd, json.dumps(payload, ensure_ascii=False), ts),
            )

    def list_outbound_queue(self, limit: int = 100) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                "SELECT id, curtain_id, cmd, payload, ts FROM outbound_queue ORDER BY id ASC LIMIT ?",
                (limit,),
            ).fetchall()
        return [dict(row) for row in rows]

    def delete_outbound(self, queue_id: int) -> None:
        with self._connect() as conn:
            conn.execute("DELETE FROM outbound_queue WHERE id=?", (queue_id,))
