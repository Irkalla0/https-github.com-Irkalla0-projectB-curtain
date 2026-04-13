# CubeMX 必选配置（匹配当前模板）

文件：`firmware/stm32_mvp_template/example_main_loop.c`

## 1. RCC / Clock

- 系统时钟：72MHz（默认稳定配置即可）

## 2. GPIO

### 输出（Push-Pull）

- `PB12`：AIN1
- `PB13`：AIN2
- `PB14`：STBY

### 输入（Pull-Up）

- `PA0`：左限位（低电平触发）
- `PA1`：右限位（低电平触发）
- `PA4`：开键（低电平触发）
- `PA5`：停键（低电平触发）
- `PA6`：关键（低电平触发）

## 3. TIM1 PWM

- `PA8` 设为 `TIM1_CH1`
- 频率建议：10kHz ~ 20kHz
- 初始占空比：0

## 4. USART1

- `PA9` TX, `PA10` RX
- 波特率：115200, 8N1

## 5. 代码生成注意点

- 生成 HAL 库工程（CubeIDE）
- 保留 `htim1` 与 `huart1` 全局句柄
- 主循环每 `10ms` 调一次 `App_Loop10ms()`

## 6. 主循环最小集成示例

```c
App_Init();
uint32_t last = HAL_GetTick();
while (1) {
    uint32_t now = HAL_GetTick();
    if (now - last >= 10) {
        last = now;
        App_Loop10ms();
    }
}
```

