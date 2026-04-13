# INMP441 语音控制网关（离线版）

这套模块用于你当前项目的最小语音增强方案：

- 只新增一个 `INMP441` 麦克风
- 语音识别运行在 `Orange Pi`（本地离线）
- STM32 继续做执行器控制
- 通过你现有网关 API 控制窗帘

## 1. 功能

- 支持命令：
  - `打开窗帘` -> 100%
  - `关闭窗帘` -> 0%
  - `停止窗帘` -> stop
  - `窗帘到50` / `窗帘到75%` / `窗帘到五十`
- 低置信度过滤（避免误触发）
- 命令冷却时间（避免重复下发）

## 2. 目录

- `voice_control.py`：主程序（采音 + ASR + 命令解析 + API调用）
- `command_parser.py`：中文命令解析
- `config.yaml`：运行配置
- `requirements.txt`：Python 依赖
- `test_command_parser.py`：解析器自测

## 3. INMP441 接线（单麦）

> 具体引脚名随 Orange Pi 设备树配置可能不同，核心是 I2S 四线。

- `INMP441 VDD` -> `3.3V`
- `INMP441 GND` -> `GND`
- `INMP441 SCK` -> `I2S_BCLK`
- `INMP441 WS` -> `I2S_LRCLK`
- `INMP441 SD` -> `I2S_DIN`
- `INMP441 L/R` -> `GND`（单麦常用左声道）

## 4. 软件安装

```bash
sudo apt update
sudo apt install -y python3-pip python3-dev portaudio19-dev libasound2-dev
python3 -m pip install -r requirements.txt
```

下载 Vosk 中文离线模型（示例）：

```bash
mkdir -p models
cd models
wget https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip
unzip vosk-model-small-cn-0.22.zip
cd ..
```

验证系统是否识别到录音设备：

```bash
arecord -l
```

若没有录音设备，先检查 I2S 驱动和设备树配置是否正确。

## 5. 配置

编辑 `config.yaml`：

- `gateway.base_url`：你本地网关地址（例：`http://127.0.0.1:8000`）
- `gateway.curtain_id`：设备 ID
- `gateway.token`：若接口有鉴权，填 JWT
- `asr.model_path`：Vosk 模型路径
- `asr.device`：音频输入设备 ID（可先留空）

## 6. 运行

先查看音频设备：

```bash
python3 voice_control.py --list-devices
```

然后运行主循环：

```bash
python3 voice_control.py --config config.yaml
```

## 7. 自测

```bash
python3 test_command_parser.py
```

## 8. 安全建议

- 语音控制仅建议在局域网网关内运行
- 开启 `token` 鉴权，不要开放匿名控制 API
- 保留实体按键急停，优先级高于语音命令
