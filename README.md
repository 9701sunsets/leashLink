# leashLink - 基于ESP32的智能遛狗安全系统

本项目设计一种基于 ESP32-C5+S3 的智能遛狗系统，通过双节点协同、边缘智能分析和主动控制机制，实现从“被动监测”到“主动防护”的升级。

## 开发

### 项圈端

操作ESP32C5
```bash
# 进入 C5 项目
cd firmware/collar_c5

# 第一次/换芯片必做
idf.py set-target esp32c5
idf.py fullclean

# 编译
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
idf.py set-target esp32s3
idf.py fullclean

# 编译、烧录
idf.py build
idf.py -p COM5 flash

# 监视
idf.py -p COM5 monitor
```

### 后端

- Python虚拟环境下进入`backend/`
- 启动mosquitto+SQL的Docker容器
```bash
uvicorn app.main:app --reload
```

### 前端

微信小程序APP