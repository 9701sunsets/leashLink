import { deviceStore } from "../../store/deviceStore";

Page({
  data: {
    events: deviceStore.events,
    filter: "all"
  },

  unsubscribe: null as null | (() => void),

  onLoad() {
    this.unsubscribe = deviceStore.subscribe(() => this.applyFilter());
    deviceStore.refreshEvents();
  },

  onUnload() {
    if (this.unsubscribe) this.unsubscribe();
  },

  setFilter(e: any) {
    this.setData({ filter: e.currentTarget.dataset.filter });
    this.applyFilter();
  },

  applyFilter() {
    const all = deviceStore.events;
    const filter = this.data.filter;
    this.setData({
      events: filter === "all" ? all : all.filter((item) => item.type === filter)
    });
  }
});

