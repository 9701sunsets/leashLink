# leashLink - 基于ESP32的智能遛狗安全系统

本项目设计一种基于 ESP32-C5+S3 的智能遛狗系统，通过双节点协同、边缘智能分析和主动控制机制，实现从“被动监测”到“主动防护”的升级。

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
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000 --log-level info --access-log
```

### 前端

微信小程序APP

## 测试

测试动作：
- 遮挡光敏：light_raw/lux/dark 应变化。
- 手指贴心率模块：ir/red 应明显变化；heart_present=1 addr=0x57 表示 I2C 已连通。
- GPS 到窗边或室外等待：gps_fix 从 0 变 1，lat/lng 出现坐标。
- C5 开机靠近/远离：S3 的 collar_rssi 应变化。
- 若接了按钮：B1 解锁舵机，B2 锁定舵机，B3 给 C5 发反馈命令并本地蜂鸣器短响。