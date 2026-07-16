import { deviceStore } from "./store/deviceStore";
import { connectRealtime } from "./services/realtime";

App({
  globalData: {
    pairId: "LL-P-0001",
    apiBaseUrl: "https://example.com/api/v1",
    wsUrl: "wss://example.com/ws"
  },

  onLaunch() {
    deviceStore.bootstrap();
    connectRealtime({
      url: this.globalData.wsUrl,
      onMessage: (message) => {
        if (message.type === "status") {
          deviceStore.setStatus(message.payload);
        }
        if (message.type === "alert") {
          deviceStore.unshiftEvent(message.event);
          wx.showModal({
            title: "安全提醒",
            content: message.event?.summary || "检测到新的安全事件",
            showCancel: false
          });
        }
      }
    });
  }
});

