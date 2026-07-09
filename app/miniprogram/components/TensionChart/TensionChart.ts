Component({
  properties: {
    values: { type: Array, value: [] }
  },
  lifetimes: {
    ready() {
      this.draw();
    }
  },
  observers: {
    values() {
      this.draw();
    }
  },
  methods: {
    draw() {
      const query = this.createSelectorQuery();
      query.select("#chart").fields({ node: true, size: true }).exec((res: any[]) => {
        const item = res[0];
        if (!item || !item.node) return;
        const canvas = item.node;
        const ctx = canvas.getContext("2d");
        const dpr = wx.getWindowInfo ? wx.getWindowInfo().pixelRatio : wx.getSystemInfoSync().pixelRatio;
        canvas.width = item.width * dpr;
        canvas.height = item.height * dpr;
        ctx.scale(dpr, dpr);
        ctx.clearRect(0, 0, item.width, item.height);
        const values = (this.data.values as number[]) || [];
        if (!values.length) return;
        const max = Math.max(25, ...values);
        const pad = 18;
        const w = item.width - pad * 2;
        const h = item.height - pad * 2;
        ctx.strokeStyle = "#E5E7EB";
        ctx.lineWidth = 1;
        for (let i = 0; i < 4; i++) {
          const y = pad + (h * i) / 3;
          ctx.beginPath();
          ctx.moveTo(pad, y);
          ctx.lineTo(item.width - pad, y);
          ctx.stroke();
        }
        ctx.strokeStyle = "#36C76C";
        ctx.lineWidth = 3;
        ctx.beginPath();
        values.forEach((v, index) => {
          const x = pad + (w * index) / Math.max(1, values.length - 1);
          const y = pad + h - (v / max) * h;
          if (index === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        });
        ctx.stroke();
      });
    }
  }
});

