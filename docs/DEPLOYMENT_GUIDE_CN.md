# ProjectB 最终版部署手册

## 1. 系统架构

ProjectB 采用四层闭环架构：

1. `STM32执行器层`：电机状态机、PWM软启停、限位/超时/故障锁定。
2. `本地网关层`：HTTPS API、JWT鉴权、UART签名下发、SQLite审计。
3. `语音输入层`：INMP441 + Vosk 离线识别，统一调用网关API。
4. `云协同层`：MQTT over TLS（本地优先，断网补传）。

## 2. 目录入口

- 固件：`firmware/cubeide_project/CurtainMVP_F103`
- 网关：`gateway/`
- 语音：`voice_control_gateway/`
- 文档：`docs/`

## 3. 固件部署（STM32）

### 3.1 一键导入与构建

```powershell
powershell -ExecutionPolicy Bypass -File D:\codex\projectB\firmware\one_click_cubeide_build.ps1
```

### 3.2 烧录与串口

- 烧录：CubeIDE Run/Debug 或 ST-Link CLI
- 串口：`115200 8N1`
- 上电日志：`# boot ok`

### 3.3 固件关键行为（最终版）

- 10ms循环顺序：读输入 -> 解析命令 -> 安全检查 -> 轨迹计算 -> 电机输出 -> 状态上报。
- 状态机：`IDLE/OPENING/CLOSING/STOPPING/CALIBRATING/FAULT_LOCKED`。
- 安全锁定：错限位、超时、堵转、防夹、过流、鉴权失败、重放攻击均可进入 `FAULT_LOCKED`。

## 4. 本地网关部署

### 4.1 配置

```bash
cp gateway/config.example.yaml gateway/config.yaml
```

根据实际环境修改：

- `gateway.uart.port`（Windows 如 `COM6`，Linux 如 `/dev/ttyUSB0`）
- `gateway.hmac.shared_key`
- `auth.jwt_secret`

### 4.2 证书

Windows:
```powershell
powershell -ExecutionPolicy Bypass -File gateway/certs/generate_dev_ca.ps1
```

Linux/macOS:
```bash
bash gateway/certs/generate_dev_ca.sh
```

### 4.3 启动

```bash
python -m pip install -r gateway/requirements.txt
python gateway/main.py --config gateway/config.yaml
```

默认地址：`https://127.0.0.1:8443`

## 5. 语音链路部署（INMP441）

```bash
bash voice_control_gateway/setup_orangepi_inmp441.sh
cp voice_control_gateway/config.example.yaml voice_control_gateway/config.yaml
python voice_control_gateway/test_command_parser.py
python voice_control_gateway/voice_control.py --list-devices
python voice_control_gateway/voice_control.py --config voice_control_gateway/config.yaml
```

## 6. 联调顺序（标准）

1. `curl` 调网关 `/target` `/stop`。
2. 检查网关串口回包是否含 `seq/result/state/current_pos/fault_code`。
3. 语音指令触发 API，下发后状态回读。
4. MQTT（可选）验证 `status/faults/replay` 主题。
