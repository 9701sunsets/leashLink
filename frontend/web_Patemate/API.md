# web_Patemate 展示信息与 API 文档

本文档根据 `frontend/web_Patemate/index.html`、`script.js`、`server.mjs` 整理，用于说明网页端展示的信息、前后端数据来源，以及当前使用的接口契约。

## 1. 页面概览

`web_Patemate` 是 PetPulse / Patemate 的网页端原型，默认通过本地 Node 服务运行：

```bash
cd frontend/web_Patemate
node server.mjs
```

访问地址：

```text
http://127.0.0.1:4173/
```

网页包含以下视图：

| 视图 | `data-view` / `section id` | 展示内容 |
| --- | --- | --- |
| 引导 | `intro` | 产品主视觉、今日状态入口、散步入口、功能摘要 |
| 今日 | `today` | 健康评分、热量进度、速度、心率、体温、人狗距离、今日建议、温柔提醒 |
| 散步 | `walk` | 有绳/无绳模式、轨迹示意、主人/狗狗位置、散步时长、消耗、平均心率、距离、心率曲线、速度曲线 |
| 记录 | `records` | 7 天散步记录、7 天便便记录 |
| 档案 | `profile` | 多只狗狗档案、新增/切换/删除狗狗、品种/年龄/体重/绝育状态、运动建议 |
| 健康助手 | `assistant` | 狗狗档案上下文、运动目标、DeepSeek/本地建议状态、聊天输入、快捷提问 |
| 提醒 | `alerts` | 牵引力度、牵引状态、安全距离 |

## 2. 本地数据模型

### 2.1 狗狗档案

网页端在 `script.js` 中维护本地数组 `dogs`：

```json
{
  "id": 1,
  "name": "Luna",
  "owner": "小林",
  "breed": "golden",
  "age": 3,
  "weight": 30,
  "neutered": false,
  "caloriesNow": 128,
  "walkMinutes": 45,
  "distanceKm": 3.2
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `id` | number | 前端本地档案 ID |
| `name` | string | 狗狗名字 |
| `owner` | string | 主人名称 |
| `breed` | string | 品种 key，对应 `breedProfiles` |
| `age` | number | 年龄 |
| `weight` | number | 体重 kg |
| `neutered` | boolean | 是否绝育 |
| `caloriesNow` | number | 今日已消耗热量 kcal |
| `walkMinutes` | number | 今日散步分钟数 |
| `distanceKm` | number | 今日散步距离 km |

### 2.2 品种档案

`breedProfiles` 维护品种运动建议：

```json
{
  "name": "金毛",
  "weightRange": [25, 34],
  "walk": [60, 90],
  "calories": [350, 500],
  "speed": "3.5-6 km/h",
  "tip": "注意髋关节和体重管理",
  "intro": "品种说明",
  "sizeFactor": 0.8
}
```

网页根据品种、年龄、体重、绝育状态计算：

| 计算项 | 说明 |
| --- | --- |
| `walkTarget` | 建议散步时长 min |
| `calorieTarget` | 建议热量消耗 kcal |
| `walkPercent` | 今日散步完成比例 |
| `safetyScore` | 今日健康/安全评分 |

## 3. 页面展示字段

### 3.1 今日状态

| 页面元素 ID | 含义 | 来源 |
| --- | --- | --- |
| `todayTitle` | 今日标题 | 当前狗狗档案 |
| `healthPill` | 健康/提醒/关注状态 | 实时指标计算 |
| `safetyScore` | 健康评分 | 本地计算 |
| `caloriesNow` | 今日已消耗热量 | `dogs[].caloriesNow` |
| `calorieTarget` | 今日目标热量 | 品种建议计算 |
| `calorieProgressBar` | 热量进度条 | `caloriesNow / calorieTarget` |
| `speedNow` | 当前速度 km/h | 本地模拟或后端状态推导 |
| `paceLabel` | 当前运动状态 | 本地模拟或 `collar.motion_state` |
| `heartRateNow` | 心率 bpm | 本地模拟 |
| `heartLabel` | 心率状态 | 本地计算 |
| `tempNow` | 体温/项圈温度 °C | 本地模拟或 `collar.temp_c_x10 / 10` |
| `tempLabel` | 温度状态 | 本地计算 |
| `distanceNow` | 人狗距离 m | 本地模拟或 `collar.distance_est_m` |
| `distanceLabel` | 距离状态 | 本地计算 |
| `walkTarget` | 建议散步时长 | 品种建议计算 |
| `recommendCalories` | 建议消耗热量 | 品种建议计算 |
| `speedTarget` | 建议速度范围 | 品种档案 |
| `todayAlertCount` | 今日提醒数量 | 本地状态 |
| `careTip` | 护理建议/光照提示 | 品种档案或 `handle.ambient_light_lux` |

### 3.2 散步页

| 页面元素 ID | 含义 |
| --- | --- |
| `walkTitle` | 散步页标题 |
| `mapDogName` | 地图中狗狗名称 |
| `walkMinutes` | 今日散步分钟 |
| `walkCalories` | 散步消耗热量 |
| `avgHeart` | 平均心率 |
| `walkDistance` | 今日散步距离 |
| `heartLine` | 心率曲线 |
| `heartRange` | 心率范围 |
| `speedLine` | 速度曲线 |
| `speedRange` | 速度范围 |

### 3.3 档案页

| 字段 | 说明 |
| --- | --- |
| `name` | 狗狗名字，可编辑 |
| `breed` | 品种，可选择 |
| `age` | 年龄，可编辑 |
| `weight` | 体重 kg，可编辑 |
| `neutered` | 是否绝育，可勾选 |
| `walk-recommend` | 该狗狗的建议散步时长和热量 |

### 3.4 提醒页

| 页面元素 ID | 含义 | 后端对应字段 |
| --- | --- | --- |
| `tensionNow` | 牵引力度 N | `handle.tension_n` |
| `tensionHint` | 牵引力度状态 | `handle.tension_n` / `handle.leash_locked` |
| `lockState` | 牵引锁定状态 | `handle.leash_locked` |
| `fenceNow` | 安全距离 | `collar.distance_est_m` |
| `fenceHint` | 距离提示 | `collar.distance_est_m` |

## 4. 后端实时状态接口

网页端每 1 秒轮询后端设备状态：

```js
const BACKEND_API_BASE = "http://127.0.0.1:8000/api/v1";
const LIVE_PAIR_ID = "LL-P-0001";
```

### 4.1 获取设备实时状态

```http
GET /api/v1/devices/{pair_id}/status
```

网页端调用：

```http
GET http://127.0.0.1:8000/api/v1/devices/LL-P-0001/status
```

响应示例：

```json
{
  "pair_id": "LL-P-0001",
  "online": true,
  "last_seen_ms": 1783334400123,
  "active_alert": "none",
  "handle": {
    "battery_pct": 82,
    "tension_n": 8.4,
    "tension_peak_n": 22.5,
    "tension_stable": true,
    "leash_locked": false,
    "ambient_light_lux": 228,
    "ambient_light_raw": 915,
    "dark": true,
    "gps": {
      "lat": 32.0609,
      "lng": 118.778,
      "fix": true,
      "accuracy_m": 4.5
    }
  },
  "collar": {
    "battery_pct": 76,
    "motion_state": "walk",
    "steps": 1523,
    "accel_peak_g": 1.4,
    "confidence_pct": 80,
    "rssi_dbm": -58,
    "temp_c_x10": 386,
    "distance_est_m": 3.2
  }
}
```

网页端使用字段：

| 响应字段 | 页面用途 |
| --- | --- |
| `handle.tension_n` | 提醒页牵引力度 |
| `handle.leash_locked` | 牵引状态 |
| `handle.ambient_light_lux` | 今日护理提示 |
| `handle.dark` | 暗光提示 |
| `collar.motion_state` | 运动状态/速度推导 |
| `collar.steps` | 推导散步分钟 |
| `collar.distance_est_m` | 今日距离、安全距离 |
| `collar.temp_c_x10` | 温度显示 |
| `collar.rssi_dbm` | 后端估距依据，当前网页不直接展示 |
| `collar.accel_peak_g` | 后续可用于异常运动提示，当前网页不直接展示 |
| `collar.confidence_pct` | 后续可用于运动识别可信度，当前网页不直接展示 |

如果后端不可用，网页保留本地模拟数据，不阻断页面预览。

## 5. 网页本地服务接口

`server.mjs` 提供静态文件服务和健康助手代理接口。

### 5.1 健康助手状态

```http
GET /api/assistant/status
```

响应：

```json
{
  "deepseek": true
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `deepseek` | boolean | 是否配置 `DEEPSEEK_API_KEY` |

页面用途：

| 页面元素 ID | 显示 |
| --- | --- |
| `assistantStatus` | `DeepSeek 已连接` 或 `本地建议` |

### 5.2 健康助手问答

```http
POST /api/assistant
Content-Type: application/json
```

请求体：

```json
{
  "question": "今天还建议散步多久",
  "dog": {
    "id": 1,
    "name": "Luna",
    "owner": "小林",
    "breed": "golden",
    "age": 3,
    "weight": 30,
    "neutered": false,
    "caloriesNow": 128,
    "walkMinutes": 45,
    "distanceKm": 3.2
  },
  "recommendation": {
    "walkTarget": 75,
    "calorieTarget": 350,
    "profile": {
      "name": "金毛",
      "speed": "3.5-6 km/h",
      "tip": "注意髋关节和体重管理",
      "intro": "品种说明"
    }
  },
  "metrics": {
    "speed": "3.8",
    "heartRate": "112",
    "temperature": "38.6",
    "distance": "3.2",
    "tension": "8.4",
    "pace": "行走"
  }
}
```

响应：

```json
{
  "answer": "建议内容",
  "mode": "deepseek",
  "deepseek": true
}
```

响应模式：

| `mode` | 含义 |
| --- | --- |
| `deepseek` | 已调用 DeepSeek 接口 |
| `local-fallback` | 未配置 key，使用本地模板回复 |
| `api-fallback` | DeepSeek 请求失败，使用本地模板回复 |
| `error-fallback` | 服务异常，使用兜底回复 |

## 6. 外部模型接口

当 `.env.local` 或环境变量中配置：

```env
DEEPSEEK_API_KEY=你的密钥
```

`server.mjs` 会请求：

```http
POST https://api.deepseek.com/chat/completions
Authorization: Bearer <DEEPSEEK_API_KEY>
Content-Type: application/json
```

请求模型：

```json
{
  "model": "deepseek-chat",
  "temperature": 0.4,
  "max_tokens": 500
}
```

## 7. 设备上报到网页的数据流

```text
ESP32-C5 项圈端
  -> ESP-NOW
ESP32-S3 手柄端
  -> MQTT leashlink/{pair_id}/telemetry
Mosquitto
  -> 后端 subscriber
FastAPI 后端内存状态
  -> GET /api/v1/devices/{pair_id}/status
web_Patemate
  -> 每 1 秒轮询并刷新页面指标
```

## 8. 当前未直接展示但后端已可提供的字段

| 字段 | 建议展示位置 |
| --- | --- |
| `handle.tension_peak_n` | 提醒页或散步曲线 |
| `handle.tension_stable` | 牵引力度状态 |
| `handle.ambient_light_raw` | 设备调试面板 |
| `handle.gps` | 散步地图真实定位 |
| `collar.accel_peak_g` | 运动异常/爆冲提醒 |
| `collar.confidence_pct` | 运动识别可信度 |
| `collar.rssi_dbm` | 设备诊断 |
| `active_alert` | 今日提醒和顶部健康状态 |

## 9. 联调命令

启动后端：

```powershell
cd D:\ESP\leashLink\backend
$env:MQTT_HOST="192.168.216.159"
$env:MQTT_PORT="1883"
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000 --log-level info --access-log
```

启动网页：

```powershell
cd D:\ESP\leashLink\frontend\web_Patemate
node server.mjs
```

测试设备状态：

```powershell
curl http://127.0.0.1:8000/api/v1/devices/LL-P-0001/status
```

测试健康助手：

```powershell
curl http://127.0.0.1:4173/api/assistant/status
```
