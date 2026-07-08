from __future__ import annotations

from app.db.repository import repository
from app.models import EventCreateRequest, EventListResponse, StoredEvent


def create_event(payload: EventCreateRequest) -> StoredEvent:
    event = StoredEvent.model_validate(payload.model_dump())
    return repository.add_event(event)


def list_events(pair_id: str, session_id: str | None = None, event_type: str | None = None) -> EventListResponse:
    return repository.list_events(pair_id=pair_id, session_id=session_id, event_type=event_type)
