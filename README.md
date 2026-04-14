# CurtainMotion-S1 Pro（项目B）

项目B是一个从 0 开始构建的智能窗帘执行器系统，目标是从 `STM32 执行器MVP` 逐步升级到 `本地网关 + 离线语音 + 安全机制` 的可演示最终版。

## 1. 项目目标

- 执行器：STM32 控制直流电机，实现开/关/停与 0-100% 开度控制
- 网关：Orange Pi 本地离线控制与边缘规则
- 安全：HTTPS/JWT（北向）+ HMAC 防重放（南向）
- 语音：INMP441 + Vosk 离线中文语音控制

## 2. 文档入口

- 总执行计划（最终版）：[docs/PROJECT_B_MASTER_EXECUTION_PLAN_CN.md](./docs/PROJECT_B_MASTER_EXECUTION_PLAN_CN.md)
- INMP441 完整接入： [docs/INMP441_OPIZERO3_COMPLETE_WORKFLOW_CN.md](./docs/INMP441_OPIZERO3_COMPLETE_WORKFLOW_CN.md)
- Day1 启动指南： [docs/START_HERE_DAY1_CN.md](./docs/START_HERE_DAY1_CN.md)
- 硬件软件清单： [docs/HARDWARE_SOFTWARE_CHECKLIST_CN.md](./docs/HARDWARE_SOFTWARE_CHECKLIST_CN.md)

## 3. 代码目录

- `firmware/`：STM32 固件工程与一键构建脚本
- `voice_control_gateway/`：Orange Pi 离线语音网关
- `docs/`：计划、接线、验收、排障文档

## 4. 快速开始

1. 先看 [Day1 启动指南](./docs/START_HERE_DAY1_CN.md)
2. 完成 [接线表](./docs/BRINGUP_WIRING_TABLE_CN.md)
3. 生成并编译固件，确认电机开/关/停
4. 按 [INMP441 流程](./docs/INMP441_OPIZERO3_COMPLETE_WORKFLOW_CN.md) 跑通语音

## 5. 当前语音命令

- `打开窗帘`
- `关闭窗帘`
- `停止窗帘`
- `窗帘到50` / `窗帘到75%` / `窗帘到五十`
