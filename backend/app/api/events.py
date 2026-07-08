from __future__ import annotations

from fastapi import APIRouter, Query

from app.models import EventCreateRequest, EventListResponse
from app.services.event_service import create_event, list_events

router = APIRouter(prefix="/events", tags=["events"])


@router.post("", response_model=EventCreateRequest)
def create_event_endpoint(payload: EventCreateRequest) -> EventCreateRequest:
    event = create_event(payload)
    return EventCreateRequest.model_validate(event.model_dump())


@router.get("", response_model=EventListResponse)
def list_events_endpoint(
    pair_id: str,
    session_id: str | None = None,
    type_: str | None = Query(default=None, alias="type"),
) -> EventListResponse:
    return list_events(pair_id=pair_id, session_id=session_id, event_type=type_)
