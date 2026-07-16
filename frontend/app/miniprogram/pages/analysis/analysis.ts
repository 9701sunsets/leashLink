import { deviceStore } from "../../store/deviceStore";

Page({
  data: {
    report: deviceStore.report
  },

  unsubscribe: null as null | (() => void),

  onLoad() {
    this.unsubscribe = deviceStore.subscribe(() => {
      this.setData({ report: deviceStore.report });
    });
    deviceStore.refreshReport();
  },

  onUnload() {
    if (this.unsubscribe) this.unsubscribe();
  }
});

