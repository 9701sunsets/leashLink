import { AlertType, MotionState, Severity } from "../types/index";

export function formatTime(ts: number): string {
  const d = new Date(ts);
  const h = `${d.getHours()}`.padStart(2, "0");
  const m = `${d.getMinutes()}`.padStart(2, "0");
  return `${h}:${m}`;
}

export function formatDate(ts: number): string {
  const d = new Date(ts);
  return `${d.getMonth() + 1}月${d.getDate()}日 ${formatTime(ts)}`;
}

export function motionLabel(state: MotionState): string {
  const map: Record<MotionState, string> = {
    idle: "静止",
    walk: "行走",
    run: "奔跑",
    burst: "爆冲",
    shake: "抖动",
    unknown: "未知"
  };
  return map[state] || "未知";
}

export function alertLabel(type: AlertType): string {
  const map: Record<AlertType, string> = {
    none: "当前安全",
    burst_pull: "爆冲",
    distance_warning: "距离过远",
    fence_breach: "围栏越界",
    fatigue_detected: "疲劳提醒",
    low_battery: "低电量",
    link_lost: "通信中断"
  };
  return map[type] || "事件";
}

export function severityClass(severity: Severity | AlertType): string {
  if (severity === "danger" || severity === "burst_pull" || severity === "fence_breach" || severity === "link_lost") {
    return "danger";
  }
  if (severity === "warning" || severity === "distance_warning" || severity === "low_battery" || severity === "fatigue_detected") {
    return "warn";
  }
  return "safe";
}

