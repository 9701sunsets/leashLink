import { deviceStore } from "../../store/deviceStore";
import { COLLAR_ID, HANDLE_ID } from "../../utils/constants";
import { sendCommand } from "../../services/device";

Page({
  data: {
    status: deviceStore.status,
    handleId: HANDLE_ID,
    collarId: COLLAR_ID
  },

  unsubscribe: null as null | (() => void),

  onLoad() {
    this.unsubscribe = deviceStore.subscribe(() => this.setData({ status: deviceStore.status }));
  },

  onUnload() {
    if (this.unsubscribe) this.unsubscribe();
  },

  async findDog() {
    await sendCommand(deviceStore.pairId, "find_dog", {
      duration_ms: 10000,
      light: "rainbow",
      buzzer: true,
      vibration: true
    });
    wx.showToast({ title: "寻狗已启动", icon: "success" });
  },

  rePair() {
    wx.showModal({
      title: "重新配对",
      content: "请长按手柄配对键，并让项圈进入广播模式。",
      showCancel: false
    });
  },

  openConfig() {
    wx.navigateTo({ url: "/pages/config/config" });
  }
});

