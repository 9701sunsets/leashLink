interface RealtimeOptions {
  url: string;
  onMessage: (message: any) => void;
}

let socketTask: any = null;

export function connectRealtime(options: RealtimeOptions) {
  if (!options.url || options.url.includes("example.com")) {
    return;
  }

  socketTask = wx.connectSocket({
    url: options.url
  });

  socketTask.onMessage((res: any) => {
    try {
      options.onMessage(JSON.parse(res.data));
    } catch (err) {
      console.warn("invalid realtime message", err);
    }
  });
}

export function closeRealtime() {
  if (socketTask) {
    socketTask.close();
    socketTask = null;
  }
}

