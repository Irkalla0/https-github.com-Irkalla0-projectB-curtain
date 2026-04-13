# Week1执行清单（从零到MVP）

## Day1：Bring-up

- [ ] 完成接线（见 `BRINGUP_WIRING_TABLE_CN.md`）
- [ ] CubeMX 生成工程
- [ ] ST-Link 下载成功
- [ ] 串口 `boot ok` 输出
- [ ] 电机正反转 + PWM 调速空载可见

## Day2：输入链路

- [ ] 3 个按键消抖（10-20ms）
- [ ] 2 个限位输入滤波
- [ ] 串口可打印按键/限位事件

## Day3：状态机初版

- [ ] 状态：Idle / Opening / Closing / Fault
- [ ] 事件：Button / Limit / Timeout
- [ ] 错误方向保护（开时误触左限位等）

## Day4：保护逻辑

- [ ] 超时保护（动作超过阈值自动停机）
- [ ] 双限位冲突判定
- [ ] 故障码框架（至少 0x01~0x03）

## Day5：MVP整体验收

- [ ] 开/关/停流程稳定
- [ ] 限位到位自动停机
- [ ] 超时触发故障
- [ ] 故障后人工复位可恢复

## Week1验收标准

1. 连续跑 30 分钟无卡死
2. 开关停命令执行正确率 >= 95%
3. 串口日志可回放关键状态变迁

## 提交建议

- `week1-day1: bringup and motor spin`
- `week1-day3: state machine v1`
- `week1-day5: mvp acceptance pass`

