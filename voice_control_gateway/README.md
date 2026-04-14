# INMP441 语音控制网关（离线）

这个模块用于把 `INMP441 + Orange Pi + 本地网关 API` 串起来，实现离线中文语音控制窗帘。

## 1. 能力范围

- 支持命令：
  - `打开窗帘` -> 100%
  - `关闭窗帘` -> 0%
  - `停止窗帘` -> stop
  - `窗帘到50` / `窗帘到75%` / `窗帘到五十` / `窗帘百分之三十`
- 低置信度过滤（避免误触发）
- 命令冷却时间（避免重复下发）

## 2. 目录说明

- `voice_control.py`：音频采集 + Vosk 识别 + 调网关 API
- `command_parser.py`：中文语音命令解析
- `config.example.yaml`：配置模板
- `test_command_parser.py`：解析器自测
- `setup_orangepi_inmp441.sh`：Orange Pi 依赖安装与模型准备脚本

## 3. 快速开始

### 3.1 安装依赖

```bash
cd voice_control_gateway
bash ./setup_orangepi_inmp441.sh
```

### 3.2 配置

```bash
cp config.example.yaml config.yaml
nano config.yaml
```

重点改这几项：

- `gateway.base_url`：本地网关地址（如 `https://127.0.0.1:8443`）
- `gateway.curtain_id`：你的设备 ID
- `gateway.token`：JWT/Token（如接口启用鉴权）
- `asr.model_path`：Vosk 模型路径
- `asr.device`：音频输入设备编号（先用 `--list-devices` 查看）

### 3.3 自测 + 运行

```bash
python3 test_command_parser.py
python3 voice_control.py --list-devices
python3 voice_control.py --config config.yaml
```

## 4. INMP441 接线（核心）

> 以 Orange Pi Zero 3 + I2S3 方案为例，详细步骤看文档：
> `../docs/INMP441_OPIZERO3_COMPLETE_WORKFLOW_CN.md`

- `VDD` -> `3.3V`
- `GND` -> `GND`
- `SCK` -> `I2S_BCLK`
- `WS` -> `I2S_LRCLK`
- `SD` -> `I2S_DIN`
- `L/R` -> `GND`（单麦常用左声道）

## 5. 安全建议

- App 只调用网关 HTTPS API，不直连 STM32。
- 网关对下位机命令保持 `HMAC + nonce + seq` 防重放。
- 语音网关建议部署在内网，开启 Token/JWT 鉴权。
