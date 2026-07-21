# leashLink - 基于ESP32的智能遛狗安全系统

本项目设计一种基于 ESP32-C5+S3 的智能遛狗系统，通过双节点协同、边缘智能分析和主动控制机制，实现从“被动监测”到“主动防护”的升级。

## 项目目录结构

```text
leashLink/
├── README.md                         # 项目总览、开发与测试入口
├── TODO.md                           # 待办事项
├── docker-compose.yml                # 本地 Mosquitto + PostgreSQL 编排
├── backend/                          # FastAPI 后端
│   ├── requirements.txt
│   └── app/
│       ├── main.py                   # FastAPI 应用入口、MQTT 消息处理
│       ├── models.py                 # API 与存储数据模型
│       ├── api/                      # REST 接口：设备、遥测、事件、配置
│       ├── services/                 # 业务服务：设备、事件、配置、会话
│       ├── db/                       # 内存/PostgreSQL 仓库与迁移 SQL
│       └── mqtt/                     # MQTT 发布与订阅封装
├── frontend/
│   ├── web_Patemate/                 # Web 原型 PetPulse
│   │   ├── index.html
│   │   ├── script.js
│   │   ├── styles.css
│   │   ├── server.mjs
│   │   └── assets/
│   └── app/                          # 微信小程序工程
│       ├── miniprogram/
│       │   ├── pages/                # index、device、map、events、profile 等页面
│       │   ├── components/           # 状态、设备、电量、图表等组件
│       │   ├── services/             # 小程序 API/实时数据服务
│       │   ├── store/                # 设备状态存储
│       │   ├── types/                # TypeScript 类型
│       │   └── utils/                # 常量、格式化、mock 数据
│       └── typings/
├── firmware/
│   ├── handle_s3/                    # 手柄端 ESP32-S3 固件
│   │   ├── main/                     # app_main、Kconfig、协议类型
│   │   ├── board/                    # 引脚与电源板级配置
│   │   ├── drivers/                  # HX711、舵机、蜂鸣、马达、GPS、OLED 等驱动
│   │   ├── services/                 # 张力、安全、牵引控制、距离、云端服务
│   │   ├── communication/            # Wi-Fi、MQTT、ESP-NOW、BLE 配对、I2C
│   │   └── tasks/                    # FreeRTOS 任务：张力、安全、云端、UI 等
│   └── collar_c5/                    # 项圈端 ESP32-C5 固件
│       └── main/
│           ├── app_main.c
│           ├── board/                # 项圈端引脚配置
│           ├── drivers/              # IMU、LED、蜂鸣、震动、扬声器等驱动
│           ├── services/             # 运动识别、电源、反馈服务
│           ├── communication/        # ESP-NOW、BLE 广播、Wi-Fi、I2C
│           └── tasks/                # IMU 与反馈任务
├── infra/
│   └── mosquitto/                    # 本地 MQTT Broker 配置与运行数据
└── tmp/                              # 临时资料、原理图截图/PDF
```

## 开发

### 项圈端

操作ESP32C5
```bash
# 进入 C5 项目
cd firmware/collar_c5

# 进入ESP扩展
& 'c:\Users\LENOVO\.vscode\extensions\espressif.esp-idf-extension-1.10.2\export.ps1'

# 第一次/换芯片必做
Remove-Item Env:IDF_TARGET -Force
idf.py set-target esp32c5
idf.py fullclean

# 编译
idf.py reconfigure
idf.py build

# 烧录（按住 BOOT→按 RST→松开 BOOT）
idf.py -p COM7 flash

# 监视
idf.py -p COM7 monitor
```

### 手柄端

操作ESP32S3
```bash
# 退回根目录或直接进 S3
cd ../../handle_s3
# 或 cd firmware/handle_s3

# 切到 S3
Remove-Item Env:IDF_TARGET -Force
idf.py set-target esp32s3
idf.py fullclean

# 编译、烧录
idf.py reconfigure
idf.py build
idf.py -p COM5 flash

# 监视
idf.py -p COM5 monitor
```

### 后端

- Python虚拟环境下进入`backend/`
- 启动mosquitto+SQL的Docker容器
```bash
cd D:\ESP\leashLink\backend
$env:MQTT_HOST="192.168.216.159"
$env:MQTT_PORT="1883"
$env:DATABASE_URL="postgresql://leashlink:leashlink123@127.0.0.1:5432/leashlink"
$env:AUTO_MIGRATE_DATABASE="1"
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000 --log-level info --access-log
```

### 前端

微信小程序APP

Web端
```bash
node server.mjs
```
打开`http://127.0.0.1:4173/`

## 测试

测试动作：
- 遮挡光敏：light_raw/lux/dark 应变化。
- 手指贴心率模块：ir/red 应明显变化；heart_present=1 addr=0x57 表示 I2C 已连通。
- GPS 到窗边或室外等待：gps_fix 从 0 变 1，lat/lng 出现坐标。
- C5 开机靠近/远离：S3 的 collar_rssi 应变化。
- 若接了按钮：B1 解锁舵机，B2 锁定舵机，B3 给 C5 发反馈命令并本地蜂鸣器短响。
