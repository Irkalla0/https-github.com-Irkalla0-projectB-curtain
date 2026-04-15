# CubeMX 必选配置（项目B）

## 1. 时钟与工程

- MCU 选择与你开发板一致（建议 STM32F103C8）
- 工程工具链选择 STM32CubeIDE
- 生成后不要删除用户代码区（USER CODE）

## 2. 外设最小配置

- `TIM1 CH1 PWM`：PA8（电机调速）
- `USART1 Asynchronous`：PA9/PA10（串口日志）
- `GPIO Output`：PB12/PB13/PB14（AIN1/AIN2/STBY）
- `GPIO Input Pull-up`：PA4/PA5/PA6（开停关按键）
- 可选限位：PA0/PA1（上拉输入）

## 3. 生成代码后必须确认

- `HAL_TIM_MODULE_ENABLED` 已启用
- `MX_TIM1_Init()` 在 `main.c` 中被调用
- 方向/使能 GPIO 默认电平正确（上电不误动作）

## 4. 建议保留

- 串口日志（状态切换、故障码、目标开度）
- 软件超时保护
- 看门狗（如果板级环境允许）
