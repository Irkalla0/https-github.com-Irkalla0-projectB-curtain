# ProjectB API 与安全手册（最终版）

## 1. 北向API（HTTPS + Bearer）

基础路径：`/api/v1/curtains/{id}`

### 1.1 设置开度

`POST /target`

请求体：

```json
{"target_pos": 50, "trace_id": "demo-1"}
```

### 1.2 停止

`POST /stop`

请求体：

```json
{"reason": "voice_command", "trace_id": "demo-2"}
```

### 1.3 查询状态

`GET /status`

### 1.4 查询故障

`GET /faults`

## 2. JWT/Token 鉴权

- Header：`Authorization: Bearer <token>`
- 支持两种模式：
  - 静态Token（本地部署快速联调）
  - JWT（HS256）

## 3. 南向串口帧（网关 -> STM32）

帧字段固定：

`device_id|cmd|target_pos|timestamp|nonce|seq|hmac`

示例：

```text
device_id=curtain-node-001|cmd=target|target_pos=50|timestamp=1710000000|nonce=abc123|seq=10|hmac=<64hex>
```

回包最少字段：

`seq|result|state|current_pos|fault_code|reason`

## 4. HMAC-SHA256 规范

签名原文（严格顺序）：

```text
device_id={device_id}|cmd={cmd}|target_pos={target_pos}|timestamp={timestamp}|nonce={nonce}|seq={seq}
```

算法：`HMAC-SHA256(shared_key, canonical_string)`，输出16进制小写。

## 5. 防重放策略

- `seq` 必须单调递增
- `nonce` 不能重复
- `timestamp` 需满足窗口校验
- 任一校验失败：拒绝命令并进入安全故障处理（设备侧）

## 6. MQTT over TLS

主题约定：

- `curtain/{id}/status`
- `curtain/{id}/faults`
- `curtain/{id}/rules`
- `curtain/{id}/replay`

策略：本地优先控制，断网缓存，恢复补传。

## 7. 审计与追溯

SQLite 存储表：

- `curtain_status`
- `fault_logs`
- `audit_logs`
- `outbound_queue`

所有北向命令和南向结果都应带 `trace_id` 或 `seq` 可追踪。
