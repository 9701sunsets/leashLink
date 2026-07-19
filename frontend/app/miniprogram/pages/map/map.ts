import { deviceStore } from "../../store/deviceStore";
import { mockTrack } from "../../utils/mock";
import { updateFence } from "../../services/device";

Page({
  data: {
    latitude: 32.0609,
    longitude: 118.778,
    markers: [] as any[],
    polyline: [] as any[],
    circles: [] as any[],
    radius: 100
  },

  onLoad() {
    this.renderMap();
  },

  renderMap() {
    const gps = deviceStore.status.handle.gps || { lat: 32.0609, lng: 118.778, fix: false, accuracy_m: 0 };
    const collar = deviceStore.status.collar;
    const latitude = gps.lat;
    const longitude = gps.lng;
    this.setData({
      latitude,
      longitude,
      markers: [
        { id: 1, latitude, longitude, title: "主人/手柄", width: 28, height: 28 },
        { id: 2, latitude: latitude + 0.0009, longitude: longitude + 0.0012, title: `Luna ${collar.distance_est_m}m`, width: 28, height: 28 }
      ],
      polyline: [
        {
          points: mockTrack,
          color: "#36C76C",
          width: 5,
          dottedLine: false
        }
      ],
      circles: [
        {
          latitude,
          longitude,
          color: "#36C76C33",
          fillColor: "#36C76C18",
          radius: this.data.radius,
          strokeWidth: 2
        }
      ]
    });
  },

  onRadiusChange(e: any) {
    this.setData({ radius: e.detail.value });
    this.renderMap();
  },

  async saveFence() {
    await updateFence(deviceStore.pairId, {
      enabled: true,
      mode: "circle",
      center: {
        lat: this.data.latitude,
        lng: this.data.longitude
      },
      radius_m: this.data.radius,
      warn_margin_m: 15
    });
    wx.showToast({ title: "围栏已保存", icon: "success" });
  }
});
