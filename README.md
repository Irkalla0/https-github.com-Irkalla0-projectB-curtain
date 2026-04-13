# CurtainMotion-S1 Pro（项目B）

基于 `STM32` 的智能窗帘执行器系统，从零开始实现并升级到 `本地网关 + 离线控制 + 边缘计算 + 安全认证`，并新增 `INMP441` 语音控制链路。

## 仓库目标

- 从 MVP 跑通执行器控制闭环
- 升级位置控制与安全保护
- 上线本地网关，支持离线控制
- 增加语音控制（INMP441 单麦）
- 提供可投递简历与可答辩的工程化材料

## 当前目录

- `docs/`：总计划、采购与软件清单、里程碑与验收标准
- `voice_control_gateway/`：INMP441 离线语音控制模块（Vosk + 网关 API）
- `deliverable_lceda*/`：硬件设计相关资料（历史版本）

## 快速开始

1. 阅读 [项目总计划](./docs/PROJECT_B_TOTAL_PLAN_CN.md)
2. 按 [硬件/软件清单](./docs/HARDWARE_SOFTWARE_CHECKLIST_CN.md) 采购和安装
3. 完成 STM32 执行器 MVP（第1-2周）
4. 部署网关并完成语音链路（第6-8周）

## 语音控制模块

语音模块位于 [voice_control_gateway/README.md](./voice_control_gateway/README.md)，支持：

- 打开窗帘
- 关闭窗帘
- 停止窗帘
- 窗帘到 50% / 75%（含中文数字）

