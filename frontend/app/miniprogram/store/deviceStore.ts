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
    await Promise.all([this.refreshStatus(), this.refreshEvents(), this.refreshReport()]);
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

