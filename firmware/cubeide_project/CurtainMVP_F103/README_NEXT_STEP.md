# Firmware Build & Flash Guide (Final)

本工程已完成与 ProjectB 最终版控制链路对齐，可直接用于编译、烧录与网关联调。

## 1. 一键导入与构建

```powershell
powershell -ExecutionPolicy Bypass -File D:\codex\projectB\firmware\one_click_cubeide_build.ps1
```

该脚本会自动执行：

1. 同步模板与工程源码
2. 自动导入 CubeIDE 工程
3. Headless 构建 Debug 版本
4. 打开 CubeIDE 工作区

## 2. 工程关键配置

- PWM: `TIM1_CH1 (PA8)`
- 电机控制: `PB12/PB13/PB14`
- 串口: `USART1 115200 8N1`
- 按键: `PA4/PA5/PA6`（低电平触发）
- 限位: `PA0/PA1`（低电平触发）

## 3. 固件能力

- 状态机：`IDLE/OPENING/CLOSING/STOPPING/CALIBRATING/FAULT_LOCKED`
- 软启动软停止
- 行程学习与掉电恢复
- 安全故障锁定与显式恢复
- UART 签名帧解析、HMAC验签、防重放

## 4. 烧录验证

- 上电日志：`# boot ok`
- 心跳日志：`# alive`
- 串口回包：`seq=...|result=...|state=...|current_pos=...|fault_code=...`
