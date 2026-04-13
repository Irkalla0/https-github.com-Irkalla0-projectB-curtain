# 项目B总计划（从零开始，8周快跑）

项目名称：`CurtainMotion-S1 Pro`  
目标：从“桌面级电机演示”升级为“可离线运行的智能窗帘执行器系统”，并包含语音控制能力。

## 1. 总体架构

- 执行器层：`STM32 + TB6612 + N20 + 限位/按键`
- 网关层：`Orange Pi + 本地API + 规则引擎 + 日志存储`
- 云端（可选）：远程查看与数据同步，不参与本地实时闭环

控制优先级：
1. 安全保护（限位/过流/堵转/防夹）
2. 本地按键/急停
3. 本地网关控制（含语音）
4. 云端命令（可选）

## 2. 8周里程碑

1. 第1周：STM32电机正反转 + PWM + 开关停
2. 第2周：双限位 + 超时保护 + 状态机初版
3. 第3周：0-100% 位置控制 + 中途改目标
4. 第4周：软启动/软停止 + 故障码体系
5. 第5周：自动校准 + 参数持久化 + 掉电恢复
6. 第6周：本地网关 API 与设备会话
7. 第7周：离线规则 + 断网缓存 + 恢复补传
8. 第8周：TLS/鉴权/HMAC 防重放 + 语音控制联调 + 验收

## 3. 语音控制（INMP441 单麦）

设计原则：只加一个 `INMP441`，识别放在网关，不在 STM32 上做 ASR。

链路：

`INMP441 -> Orange Pi(离线识别) -> 命令解析 -> /api/v1/curtains/{id}/target|stop`

命令集：

- 打开窗帘（100%）
- 关闭窗帘（0%）
- 停止窗帘
- 窗帘到 50% / 75% / 五十

实现目录：`voice_control_gateway/`

## 4. 公共接口（固定）

### 网关北向 API

- `POST /api/v1/curtains/{id}/target`
- `POST /api/v1/curtains/{id}/stop`
- `GET /api/v1/curtains/{id}/status`
- `GET /api/v1/curtains/{id}/faults`

### 执行器南向指令字段

`device_id`, `cmd`, `target_pos`, `timestamp`, `nonce`, `seq`, `hmac`, `fw_version`

## 5. 安全基线

- App -> 网关：HTTPS + Token/JWT
- 网关 -> 云：MQTT over TLS（可选 mTLS）
- 网关 -> 执行器：HMAC-SHA256 + 时间窗 + 防重放（seq/nonce）

## 6. 验收标准（最小）

- 功能：开/关/停、开度控制、断网可控
- 安全：未授权请求拒绝、重放请求拒绝
- 可靠性：掉电恢复、故障码可追溯
- 语音：四条命令可稳定触发，误触发可控

## 7. GitHub同步策略

- 每周至少一次里程碑提交
- 提交信息格式：`weekX: <核心功能>`
- `docs/` 与 `voice_control_gateway/` 保持同步更新
- 验收前输出 `CHANGELOG.md`（可选）

