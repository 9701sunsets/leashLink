from __future__ import annotations

from app.db.repository import repository
from app.models import DeviceConfigRequest, DeviceConfigResponse, FenceConfigRequest, FenceConfigResponse

# 设备围栏配置设置服务模块
def set_fence(pair_id: str, fence: FenceConfigRequest) -> FenceConfigResponse:
    version = repository.upsert_fence(pair_id, fence)
    return FenceConfigResponse(saved=True, version=version)

# 设备配置设置服务模块
def set_device_config(pair_id: str, config: DeviceConfigRequest) -> DeviceConfigResponse:
    version = repository.upsert_config(pair_id, config)
    return DeviceConfigResponse(saved=True, config_version=version)
