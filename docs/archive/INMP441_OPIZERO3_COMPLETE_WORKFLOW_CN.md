# INMP441 + Orange Pi Zero 3 完整接入流程（项目B）

> 目标：把 `INMP441（I2S）` 接入 Orange Pi Zero 3，跑通本地离线语音识别，并调用项目B网关 API 控制窗帘。

## 0. 先说结论（避免走弯路）

- Orange Pi Zero 3 官方 26Pin 默认功能里没有直接开箱即用的 I2S 麦克风输入。
- 要让 INMP441 可用，需要启用/加载 `I2S3` 相关设备树 overlay。
- 本文给你的是可落地路线：先连线 -> 启用 I2S3 -> 验证录音 -> 跑 `voice_control_gateway`。

## 1. 硬件接线

### 1.1 线材与供电要求

- INMP441 必须接 `3.3V`，不能接 5V。
- Orange Pi 与 INMP441 必须共地。
- 建议用短杜邦线，尽量避免电源线和时钟线过长。

### 1.2 接线表（Orange Pi Zero 3，I2S3）

| INMP441 引脚 | Orange Pi Zero 3 | 说明 |
|---|---|---|
| VDD | 3.3V（Pin 1 或 Pin 17） | 电源 |
| GND | GND（Pin 6/9/14/20/25 任一） | 地 |
| SCK | PH6（Pin 23） | I2S3 BCLK |
| WS | PH7（Pin 19） | I2S3 LRCK/WS |
| SD | PH9（Pin 24） | I2S3 DIN（麦克风数据输入） |
| L/R | GND | 单麦默认左声道 |

## 2. 系统准备

### 2.1 更新系统

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y git nano alsa-utils
```

### 2.2 进入语音模块目录

```bash
cd ~/projectB/voice_control_gateway
```

> 如果你项目不在这个路径，换成你的实际路径。

## 3. 启用 Orange Pi Zero 3 的 I2S3

### 3.1 编辑启动配置

```bash
sudo nano /boot/orangepiEnv.txt
```

确认/补充：

```ini
overlay_prefix=sun50i-h616
overlays=i2s3
```

保存后重启：

```bash
sudo reboot
```

### 3.2 如果系统没有 `i2s3` overlay

有些镜像没有内置该 overlay，需要手动放入 dtbo 文件。可参考社区仓库：

- `elkoni/Opi_Zero_3_I2S3_5.4`
- `elkoni/Opi_Zero_3_I2S3_6.1`

然后把 `sun50i-h616-i2s3.dtbo` 放到系统 overlay 目录，再重启。

## 4. 验证 I2S 麦克风是否被系统识别

### 4.1 查看录音设备

```bash
arecord -l
```

目标：能看到 `ahubi2s3` 或者可用的 capture 设备。

### 4.2 录一段 5 秒音频

```bash
arecord -D hw:3,0 -f S16_LE -r 16000 -c 1 -d 5 test.wav
```

> `hw:3,0` 只是示例，实际以 `arecord -l` 列出的卡号为准。

## 5. 安装语音网关依赖

```bash
cd ~/projectB/voice_control_gateway
bash ./setup_orangepi_inmp441.sh
```

## 6. 配置项目B语音控制

```bash
cp config.example.yaml config.yaml
nano config.yaml
```

最关键字段：

- `gateway.base_url`: 你的本地网关地址
- `gateway.curtain_id`: 你的窗帘设备 ID
- `gateway.token`: 如果网关启用了 JWT/Token 就填
- `asr.model_path`: Vosk 模型目录
- `asr.device`: 麦克风设备编号（可先留 `null`）

## 7. 自测 + 启动

### 7.1 解析器自测

```bash
python3 test_command_parser.py
```

### 7.2 列设备

```bash
python3 voice_control.py --list-devices
```

### 7.3 启动语音控制

```bash
python3 voice_control.py --config config.yaml
```

## 8. 联调顺序（强烈建议按这个顺序）

1. 先 `curl` 手动调一次网关 `/target`、`/stop`，确认 API 可用。
2. 再跑语音识别，看终端里是否有 `[ASR] text=...`。
3. 最后看是否出现 `[OK] cmd=...`，并观察窗帘动作。

## 9. 常见问题排查

### 9.1 `arecord -l` 没有设备

- 检查 `orangepiEnv.txt`（拼写、overlay 名称）
- 检查是否真的加载了 dtbo
- 检查线序（尤其 SD 是否接到 `PH9`）

### 9.2 有识别文本，但不执行动作

- 看 `config.yaml` 的 `gateway.base_url`、`token` 是否正确
- 检查网关 API 是否返回 2xx

### 9.3 一直 `[SKIP] low confidence`

- 降低 `asr.min_confidence`，例如 `0.55`
- 麦克风靠近说话者，先用安静环境测试

### 9.4 识别中文正常但命令不触发

- 先执行 `python3 test_command_parser.py`
- 用标准命令词验证：`打开窗帘`、`关闭窗帘`、`停止窗帘`、`窗帘到五十`

## 10. 完成标准（验收）

满足以下 5 条算你这一步完成：

- `arecord -l` 可见输入设备
- `python3 test_command_parser.py` 通过
- `python3 voice_control.py --list-devices` 可见麦克风
- 语音命令能触发至少 3 种动作（开/关/停）
- 日志中出现 `[OK]` 且窗帘执行动作
