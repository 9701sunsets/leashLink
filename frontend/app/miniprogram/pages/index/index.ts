import { deviceStore } from "../../store/deviceStore";
import { alertLabel, motionLabel, severityClass } from "../../utils/formatter";

Page({
  data: {
    status: deviceStore.status,
    report: deviceStore.report,
    safetyLabel: "当前安全",
    safetyTone: "safe",
    motionText: "行走",
    walking: false
  },

  unsubscribe: null as null | (() => void),

  onLoad() {
    this.unsubscribe = deviceStore.subscribe(() => this.sync());
    this.sync();
  },

  onShow() {
    deviceStore.refreshStatus();
  },

  onUnload() {
    if (this.unsubscribe) this.unsubscribe();
  },

  sync() {
    const status = deviceStore.status;
    this.setData({
      status,
      report: deviceStore.report,
      safetyLabel: alertLabel(status.active_alert),
      safetyTone: severityClass(status.active_alert),
      motionText: motionLabel(status.collar.motion_state)
    });
  },

  toggleWalk() {
    const walking = !this.data.walking;
    this.setData({ walking });
    wx.showToast({
      title: walking ? "已开始遛狗" : "已结束遛狗",
      icon: "success"
    });
  },

  openEvents() {
    wx.switchTab({ url: "/pages/events/events" });
  },

  openConfig() {
    wx.navigateTo({ url: "/pages/config/config" });
  }
});

