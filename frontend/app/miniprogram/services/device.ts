import { request } from "./api";
import { DeviceConfig, DeviceStatus, WalkReport } from "../types/index";
import { mockConfig, mockReport, mockStatus } from "../utils/mock";

const USE_MOCK = false;

export async function getDeviceStatus(pairId: string): Promise<DeviceStatus> {
  if (USE_MOCK) return mockStatus;
  try {
    return await request<DeviceStatus>(`/devices/${pairId}/status`);
  } catch {
    return mockStatus;
  }
}

export async function getWalkReport(pairId: string): Promise<WalkReport> {
  if (USE_MOCK) return mockReport;
  return request<WalkReport>(`/devices/${pairId}/report/today`);
}

export async function getDeviceConfig(pairId: string): Promise<DeviceConfig> {
  if (USE_MOCK) return mockConfig;
  return request<DeviceConfig>(`/devices/${pairId}/config`);
}

export async function updateDeviceConfig(pairId: string, config: DeviceConfig): Promise<{ saved: boolean; config_version: number }> {
  if (USE_MOCK) return { saved: true, config_version: Date.now() };
  return request(`/devices/${pairId}/config`, "PUT", config);
}

export async function updateFence(pairId: string, payload: unknown): Promise<{ saved: boolean; version: number }> {
  if (USE_MOCK) return { saved: true, version: Date.now() };
  return request(`/devices/${pairId}/fence`, "PUT", payload);
}

export async function sendCommand(pairId: string, type: string, payload: unknown): Promise<{ accepted: boolean }> {
  if (USE_MOCK) return { accepted: true };
  return request(`/devices/${pairId}/commands`, "POST", { type, payload });
}
