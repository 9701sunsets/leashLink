import { API_BASE_URL } from "../utils/constants";

type HttpMethod = "GET" | "POST" | "PUT" | "DELETE";

export function request<T>(path: string, method: HttpMethod = "GET", data?: unknown): Promise<T> {
  const token = wx.getStorageSync("token") || "dev-token";
  return new Promise((resolve, reject) => {
    wx.request({
      url: `${API_BASE_URL}${path}`,
      method,
      data,
      header: {
        Authorization: `Bearer ${token}`,
        "Content-Type": "application/json"
      },
      success(res: any) {
        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve(res.data as T);
        } else {
          reject(new Error(`HTTP ${res.statusCode}`));
        }
      },
      fail: reject
    });
  });
}

