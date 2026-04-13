# MVP 状态机仿真（先在PC上跑逻辑）

这个目录用于在烧录前先验证“动作逻辑是否合理”，避免一上板就混乱。

## 运行

```bash
python curtain_mvp_sim.py
```

## 支持事件

- `open`：打开窗帘
- `close`：关闭窗帘
- `stop`：停止
- `left_limit`：左限位触发
- `right_limit`：右限位触发
- `tick`：周期调度（模拟时间推进）
- `fault`：强制故障
- `reset`：故障复位
- `status`：查看状态
- `quit`：退出

## 目标

先把状态机和保护流程在 PC 上走通，再迁移到 STM32 代码。

