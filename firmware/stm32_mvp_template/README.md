# STM32 执行器控制模板（Final）

本模板用于维护 ProjectB 固件核心逻辑，已对齐最终版执行器行为与网关安全协议。

## 1. 模块说明

- `curtain_ctrl.h/.c`：状态机、轨迹控制、安全守护
- `curtain_port.h`：硬件抽象接口（电机输出、输入采集、日志）
- `app_entry.h/.c`：HAL适配、UART签名帧处理、持久化、调度入口

## 2. 核心数据字段

- `current_pos` / `target_pos`
- `motion_dir` / `pwm_now` / `pwm_target`
- `travel_ticks_full` / `timeout_ticks`
- `fault_code` / `recoverable`

## 3. 状态机

`IDLE -> OPENING/CLOSING -> STOPPING -> IDLE`

异常路径：任意运动状态 -> `FAULT_LOCKED`（显式 `reset_fault` 才能恢复）

校准路径：`CALIBRATING`（开限位->关限位学习行程）

## 4. 安全机制

- 错限位、超时、堵转、防夹、过流故障锁定
- 网关下发命令执行 HMAC-SHA256 验签
- 防重放：`timestamp + nonce + seq`

## 5. 集成方式

将模板文件同步到 CubeIDE 工程后，由 `App_Init` / `App_Run10msScheduler` 驱动。
