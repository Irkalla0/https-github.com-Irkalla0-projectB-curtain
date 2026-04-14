# 项目B最终版总落地计划（从0开始，去重版）

> 写法说明：按“目标 -> 接线/配置 -> 命令 -> 验收 -> 故障排查”统一输出，和语音控制章节同风格。
> 周期：8周快跑（原版 -> 升级版 -> Pro版）。

## 0. 最终目标

- STM32 执行器可稳定控制窗帘（0-100% 开度）。
- Orange Pi 本地网关可离线控制并执行边缘规则。
- 安全链路完整：HTTPS/mTLS（可选）+ JWT/Token + HMAC 防重放。
- INMP441 离线语音可直接执行开/关/停/目标开度。
- 可用于答辩/简历：有演示、有日志、有验收记录。

## 1. 一次性准备（Day 0）

### 1.1 必买硬件（最低预算）

- STM32 开发板 x1
- TB6612FNG 驱动板 x1
- 直流减速电机（两线）x1
- 电机独立电源（5V/2A 以上）x1
- Orange Pi Zero 3（1GB）x1
- INMP441 x1
- MicroSD（16GB+）x1 + 读卡器 x1
- CH340 USB-TTL x1
- 杜邦线/面包板/联轴器或摩擦轮/支架若干

### 1.2 必装软件

- PC：STM32CubeMX、STM32CubeIDE、Git、串口工具
- Orange Pi：Armbian（推荐）+ Python3 + pip + alsa-utils
- 项目依赖：requests/sounddevice/vosk/pyyaml

## 2. 第1-2周（原版MVP）

## 2.1 接线（执行器最小闭环）

- STM32 -> TB6612：`AIN1/AIN2/PWMA/STBY/GND`
- TB6612 -> 电机：`A01/A02`
- TB6612 -> 电源：`VM=5V`，`VCC=3.3V`，必须共地

## 2.2 固件目标

- 开/关/停状态机
- PWM 调速
- 超时保护
- 串口日志（115200）

## 2.3 验收

- 开/关/停 3 条命令均可执行
- 连续运行 10 分钟无死机
- 串口能看到状态切换日志

## 3. 第3-5周（升级版）

## 3.1 控制能力升级

- 开关控制升级为 `0-100%` 目标开度
- 加入软启动/软停止

## 3.2 自校准与持久化

- 行程学习（自动校准）
- Flash 参数持久化（掉电恢复）

## 3.3 保护机制

- 堵转保护
- 防夹保护
- 过流保护
- 故障码上报

## 3.4 验收

- 目标开度控制稳定
- 掉电重启后恢复位置
- 触发故障后能锁定并可复位

## 4. 第6-8周（Pro版）

## 4.1 本地网关

- Orange Pi 部署本地网关
- API 统一为：`/target`、`/stop`、`/status`、`/faults`
- 断网可本地控制，支持缓存补传

## 4.2 安全机制

- App -> 网关：`HTTPS + Token/JWT`
- 网关 -> 执行器：`HMAC-SHA256 + nonce + seq + timestamp`
- 禁止 App 直连 STM32 业务口

## 4.3 语音能力（INMP441）

- INMP441 接 Orange Pi I2S
- 离线 ASR（Vosk）
- 语音命令映射：开/关/停/百分比

## 4.4 验收

- 未授权请求拒绝
- 重放包拒绝
- 断网场景语音仍可控制本地设备

## 5. INMP441完整执行步骤（直接照做）

## 5.1 接线（Orange Pi Zero 3，I2S3）

- `VDD -> 3.3V`
- `GND -> GND`
- `SCK -> PH6(Pin23)`
- `WS -> PH7(Pin19)`
- `SD -> PH9(Pin24)`
- `L/R -> GND`

## 5.2 系统配置

```bash
sudo nano /boot/orangepiEnv.txt
```

确保有：

```ini
overlay_prefix=sun50i-h616
overlays=i2s3
```

重启：

```bash
sudo reboot
```

## 5.3 依赖安装与模型下载

```bash
cd ~/projectB/voice_control_gateway
bash ./setup_orangepi_inmp441.sh
```

## 5.4 运行前检查

```bash
arecord -l
python3 test_command_parser.py
python3 voice_control.py --list-devices
```

## 5.5 启动语音控制

```bash
cp config.example.yaml config.yaml
nano config.yaml
python3 voice_control.py --config config.yaml
```

## 6. 每周固定交付（防止只开发不收敛）

每周都要提交下面 4 样东西：

1. 功能演示视频（1个主流程）
2. 关键日志（成功+失败各1份）
3. 回归结论（原功能是否退化）
4. 下周Top3动作（只写3条）

## 7. 风险与回退

- 电机不转：先查独立电源，再查 STBY/PWM，再查共地
- 串口没日志：先查 CH340 驱动与端口占用
- I2S无设备：先查 overlay，再查线序（尤其 SD）
- 语音误触发：提高置信度阈值并加命令冷却
- 鉴权失败：先 curl API，再联调语音

## 8. 简历写法（按完成度）

### 原版（1-2周）

`基于STM32实现智能窗帘执行器MVP，完成电机PWM调速、开/关/停状态机与超时保护。`

### 升级版（3-5周）

`实现0-100%开度控制、软启停、自动校准与参数持久化，建立堵转/防夹/过流保护闭环。`

### Pro版（6-8周）

`搭建Orange Pi本地网关实现离线控制与边缘规则，集成INMP441离线语音控制，并完成HTTPS/JWT/HMAC防重放安全联调。`

## 9. 今天就做（最短路径）

1. `bash ./setup_orangepi_inmp441.sh`
2. `arecord -l`
3. `python3 test_command_parser.py`
4. `python3 voice_control.py --list-devices`
5. `python3 voice_control.py --config config.yaml`
