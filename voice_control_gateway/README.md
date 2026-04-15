# Voice Control Gateway (INMP441, Offline)

本模块是 ProjectB 最终版的语音输入端，负责离线识别中文指令并调用本地网关 HTTPS API。

## 1. 功能

- 离线识别（Vosk）
- 命令解析：打开/关闭/停止/百分比/中文数字
- 低置信度过滤与冷却时间控制
- 与本地网关统一接口：`/target` `/stop`

## 2. 快速启动

```bash
bash ./setup_orangepi_inmp441.sh
cp config.example.yaml config.yaml
python3 test_command_parser.py
python3 voice_control.py --list-devices
python3 voice_control.py --config config.yaml
```

## 3. 配置要点

- `gateway.base_url`: `https://<gateway-host>:8443`
- `gateway.curtain_id`: 设备ID
- `gateway.token`: Bearer Token/JWT
- `gateway.verify_tls`: 开启TLS证书校验时设为 `true`
- `asr.model_path`: Vosk 中文模型路径
- `asr.device`: 麦克风输入设备编号

## 4. INMP441 接线（Orange Pi Zero 3）

- `VDD -> 3.3V`
- `GND -> GND`
- `SCK -> PH6 (Pin23)`
- `WS -> PH7 (Pin19)`
- `SD -> PH9 (Pin24)`
- `L/R -> GND`

## 5. 验收命令

- `打开窗帘`
- `关闭窗帘`
- `停止窗帘`
- `窗帘到五十` / `窗帘到75%`
