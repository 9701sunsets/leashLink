import { DeviceStatus, SafetyEvent, WalkReport } from "../types/index";
import { PAIR_ID } from "../utils/constants";
import { getDeviceStatus, getWalkReport } from "../services/device";
import { getEvents } from "../services/event";
import { mockReport, mockStatus } from "../utils/mock";

type Listener = () => void;

class DeviceStore {
  pairId = PAIR_ID;
  status: DeviceStatus = mockStatus;
  events: SafetyEvent[] = [];
  report: WalkReport = mockReport;
  private listeners: Listener[] = [];
  private pollTimer: number | null = null;
  private pollBusy = false;
  private reportRefreshTick = 0;

  subscribe(listener: Listener) {
    this.listeners.push(listener);
    return () => {
      this.listeners = this.listeners.filter((item) => item !== listener);
    };
  }

  notify() {
    this.listeners.forEach((listener) => listener());
  }

  async bootstrap() {
    await Promise.all([
      this.refreshStatus().catch(() => undefined),
      this.refreshEvents().catch(() => undefined),
      this.refreshReport().catch(() => undefined)
    ]);
    this.startPolling();
  }

  startPolling(intervalMs = 1000) {
    if (this.pollTimer !== null) return;
    this.pollTimer = setInterval(() => {
      void this.refreshLiveData();
    }, intervalMs) as unknown as number;
  }

  stopPolling() {
    if (this.pollTimer === null) return;
    clearInterval(this.pollTimer);
    this.pollTimer = null;
  }

  private async refreshLiveData() {
    if (this.pollBusy) return;
    this.pollBusy = true;
    try {
      await this.refreshStatus();
      this.reportRefreshTick += 1;
      if (this.reportRefreshTick >= 5) {
        this.reportRefreshTick = 0;
        await this.refreshReport().catch(() => undefined);
      }
    } finally {
      this.pollBusy = false;
    }
  }

  async refreshStatus() {
    this.status = await getDeviceStatus(this.pairId);
    this.notify();
  }

  async refreshEvents() {
    this.events = await getEvents(this.pairId);
    this.notify();
  }

  async refreshReport() {
    this.report = await getWalkReport(this.pairId);
    this.notify();
  }

  setStatus(status: DeviceStatus) {
    this.status = status;
    this.notify();
  }

  unshiftEvent(event: SafetyEvent) {
    this.events = [event, ...this.events].slice(0, 50);
    this.notify();
  }
}

export const deviceStore = new DeviceStore();
