# STM32 MVP 固件模板（可移植到 CubeIDE）

这是一套“从零开工”的最小执行器控制模板，目标：

- 开/关/停控制
- 双限位保护
- 超时保护
- 故障锁定与复位
- 10ms 周期调度

## 目录

- `curtain_ctrl.h/.c`：状态机与控制逻辑
- `curtain_port.h`：硬件抽象接口（你在 Cube 工程里实现）
- `app_entry.h/.c`：可直接集成到 Cube 工程的入口与调度
- `example_main_loop.c`：参考示例（可不使用）

## 接入步骤（CubeIDE）

1. 在 CubeMX 配好 GPIO/TIM/UART（按 docs 接线表）
2. 将 `curtain_ctrl.*`, `curtain_port.h`, `app_entry.*` 拷入工程（例如 `Core/Src` 和 `Core/Inc`）
3. 在 `main.c` 中包含 `app_entry.h`
4. 在初始化后调用 `App_Init()`
5. 在 `while(1)` 中持续调用 `App_Run10msScheduler()`

CubeMX 配置可直接参考：

- `docs/CUBEMX_REQUIRED_CONFIG_CN.md`

## Day1 最小验证

1. 上电打印 `boot ok`
2. 按“开键”后电机朝开方向运动
3. 触发右限位后自动停机
4. 超时未到位进入故障态
5. 长按“停键”触发复位（可在你的按键层实现）
