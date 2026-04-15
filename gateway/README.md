# ProjectB 本地网关（最终版）

本目录提供 `HTTPS API + JWT鉴权 + UART下行 + HMAC防重放 + SQLite审计 + MQTT over TLS` 的完整本地网关实现。

## 1. 功能

- 北向 API：`/target` `/stop` `/status` `/faults`
- 鉴权：Bearer Token / JWT
- 南向：UART 帧签名（HMAC-SHA256）
- 防重放：`timestamp + nonce + seq`
- 存储：SQLite 状态/故障/审计/补传队列
- 云协同：MQTT over TLS（可开关）

## 2. 快速启动

```bash
cp gateway/config.example.yaml gateway/config.yaml
python -m pip install -r gateway/requirements.txt
python gateway/main.py --config gateway/config.yaml
```

默认监听：`https://0.0.0.0:8443`

一键启动脚本：

- Windows: `powershell -ExecutionPolicy Bypass -File gateway/run_gateway.ps1`
- Linux/macOS: `bash gateway/run_gateway.sh`

## 3. 证书（开发CA）

Linux/macOS:
```bash
bash gateway/certs/generate_dev_ca.sh
```

Windows PowerShell:
```powershell
powershell -ExecutionPolicy Bypass -File gateway/certs/generate_dev_ca.ps1
```

## 4. API 示例

```bash
curl -k -X POST "https://127.0.0.1:8443/api/v1/curtains/living-room/target" \
  -H "Authorization: Bearer projectb-local-token" \
  -H "Content-Type: application/json" \
  -d '{"target_pos":50,"trace_id":"demo-1"}'
```

## 5. 测试

```bash
pytest gateway/tests -q
```
