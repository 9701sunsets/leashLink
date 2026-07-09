import { alertLabel, formatDate, severityClass } from "../../utils/formatter";

Component({
  properties: {
    event: Object
  },
  data: {
    label: "",
    timeText: "",
    tone: "safe"
  },
  observers: {
    event(event) {
      if (!event) return;
      this.setData({
        label: alertLabel(event.type),
        timeText: formatDate(event.ts_ms),
        tone: severityClass(event.severity)
      });
    }
  }
});

