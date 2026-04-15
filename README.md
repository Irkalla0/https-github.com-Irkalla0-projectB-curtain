# ProjectB CurtainMotion-S1 Pro Final Release

ProjectB 是面向嵌入式/IoT 场景的智能窗帘执行器最终版工程，交付形态为“可运行代码 + 可复现流程 + 可验收证据”。

## 1. 最终版能力

- STM32 电机执行器：0-100% 开度、软启停、自动校准、掉电恢复、故障锁定
- 本地网关服务：HTTPS API、JWT/Token、UART签名下发、SQLite审计与补传
- 安全机制：HMAC-SHA256、`timestamp+nonce+seq` 防重放、安全故障闭环
- 语音控制：INMP441 + Vosk 离线中文控制（开/关/停/百分比）
- 云协同：MQTT over TLS（本地优先控制）

## 2. 文档入口（对外交付）

- 部署手册：[docs/DEPLOYMENT_GUIDE_CN.md](./docs/DEPLOYMENT_GUIDE_CN.md)
- 接线手册：[docs/WIRING_GUIDE_CN.md](./docs/WIRING_GUIDE_CN.md)
- API与安全手册：[docs/API_SECURITY_GUIDE_CN.md](./docs/API_SECURITY_GUIDE_CN.md)
- 验收报告：[docs/ACCEPTANCE_REPORT_CN.md](./docs/ACCEPTANCE_REPORT_CN.md)

> 历史过程资料已归档至 `docs/archive/`。

## 3. 代码结构

- `firmware/`：STM32 CubeIDE 固件工程（执行器主控）
- `gateway/`：本地网关服务端（HTTPS + UART + SQLite + MQTT）
- `voice_control_gateway/`：离线语音输入与命令解析
- `docs/`：交付文档

## 4. 一键起步

### 4.1 固件构建

```powershell
powershell -ExecutionPolicy Bypass -File D:\codex\projectB\firmware\one_click_cubeide_build.ps1
```

### 4.2 网关启动

```bash
cp gateway/config.example.yaml gateway/config.yaml
python -m pip install -r gateway/requirements.txt
python gateway/main.py --config gateway/config.yaml
```

### 4.3 语音启动

```bash
bash voice_control_gateway/setup_orangepi_inmp441.sh
python voice_control_gateway/voice_control.py --config voice_control_gateway/config.yaml
```

## 5. API 快速示例

```bash
curl -k -X POST "https://127.0.0.1:8443/api/v1/curtains/living-room/target" \
  -H "Authorization: Bearer projectb-local-token" \
  -H "Content-Type: application/json" \
  -d '{"target_pos":50,"trace_id":"demo-1"}'
```
