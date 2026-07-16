import { deviceStore } from "../../store/deviceStore";
import { getDeviceConfig, updateDeviceConfig } from "../../services/device";
import { DeviceConfig } from "../../types/index";

Page({
  data: {
    config: null as null | DeviceConfig
  },

  async onLoad() {
    const config = await getDeviceConfig(deviceStore.pairId);
    this.setData({ config });
  },

  setNumber(path: string, value: number) {
    const config = this.data.config as DeviceConfig;
    const next = JSON.parse(JSON.stringify(config));
    const keys = path.split(".");
    let cursor = next;
    while (keys.length > 1) {
      cursor = cursor[keys.shift() as string];
    }
    cursor[keys[0]] = value;
    this.setData({ config: next });
  },

  onSlider(e: any) {
    this.setNumber(e.currentTarget.dataset.path, e.detail.value);
  },

  onSwitch(e: any) {
    const config = this.data.config as DeviceConfig;
    const next = JSON.parse(JSON.stringify(config));
    next.night_mode.enabled = e.detail.value;
    this.setData({ config: next });
  },

  async save() {
    if (!this.data.config) return;
    await updateDeviceConfig(deviceStore.pairId, this.data.config);
    wx.showToast({ title: "配置已保存", icon: "success" });
  }
});

