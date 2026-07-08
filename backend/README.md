# leashLink backend

这是 leashLink 项目的云端层实现，基于 FastAPI 提供设备注册、实时状态、遥测上报、事件查询、围栏配置和设备配置接口。

默认实现使用内存仓库保存数据，方便本地快速启动和联调；MQTT 发布/订阅为可选能力，只有在配置了相关环境变量并且安装依赖后才会连接。

## 目录结构

- `app/main.py`：FastAPI 入口，注册路由和生命周期钩子。
- `app/api/`：REST 接口层。
- `app/services/`：业务服务层。
- `app/db/`：数据仓库与数据库连接封装。
- `app/mqtt/`：MQTT 发布/订阅封装。

## 环境要求

- Python 3.13+
- 建议使用独立虚拟环境

## 安装依赖

```bash
pip install -r requirements.txt
```

## 启动服务

```bash
uvicorn app.main:app --reload
```

默认监听地址：`http://127.0.0.1:8000`

健康检查：`GET /health`

## API 前缀

所有业务接口都挂载在 `/api/v1` 下。

### 设备

- `POST /api/v1/devices/register`：注册设备并生成 `pair_id`
- `GET /api/v1/devices/{pair_id}/status`：查询设备状态
- `PUT /api/v1/devices/{pair_id}/fence`：配置电子围栏
- `PUT /api/v1/devices/{pair_id}/config`：下发设备配置

### 遥测与事件

- `POST /api/v1/telemetry`：上传遥测
- `POST /api/v1/events`：上传事件
- `GET /api/v1/events`：查询事件列表

## 环境变量

- `MQTT_HOST`：MQTT 服务地址
- `MQTT_PORT`：MQTT 端口，默认 `1883`
- `DATABASE_URL`：PostgreSQL 连接串

## 当前实现说明

- 设备、遥测、事件、配置接口已经可用。
- 默认仓库是内存实现，重启后数据不会保留。
- PostgreSQL 连接封装已预留，但还没有替换为完整持久化仓库。
- MQTT 功能是可选的，不配置时会自动退化为无操作。

## 后续建议

1. 把内存仓库替换成 PostgreSQL 持久化。
2. 增加鉴权、设备密钥和用户登录态校验。
3. 把 MQTT topic 和设备事件流进一步打通。

