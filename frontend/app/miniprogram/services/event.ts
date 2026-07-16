import { request } from "./api";
import { SafetyEvent } from "../types/index";
import { mockEvents } from "../utils/mock";

const USE_MOCK = true;

export async function getEvents(pairId: string, type?: string): Promise<SafetyEvent[]> {
  if (USE_MOCK) {
    return type ? mockEvents.filter((item) => item.type === type) : mockEvents;
  }
  const query = type ? `&type=${type}` : "";
  const res = await request<{ items: SafetyEvent[] }>(`/events?pair_id=${pairId}${query}`);
  return res.items;
}

