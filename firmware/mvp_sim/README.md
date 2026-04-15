# MVP 状态机仿真工具

该目录提供执行器控制逻辑的 PC 侧快速仿真，用于验证状态机行为与保护动作。

## 运行

```bash
python curtain_mvp_sim.py
```

## 支持事件

- `open` / `close` / `stop`
- `left_limit` / `right_limit`
- `fault` / `reset`
- `tick` / `status` / `quit`

## 用途

- 在不上板条件下快速验证控制逻辑
- 复盘异常状态切换
- 对照固件行为进行回归检查
