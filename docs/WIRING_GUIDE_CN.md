# ProjectB 最终版接线手册

## 1. STM32 与 TB6612FNG

| 功能 | STM32 引脚 | TB6612FNG 引脚 | 说明 |
|---|---|---|---|
| 电机方向1 | PB12 | AIN1 | 正反转控制 |
| 电机方向2 | PB13 | AIN2 | 正反转控制 |
| PWM调速 | PA8(TIM1_CH1) | PWMA | 0-100%占空比 |
| 使能 | PB14 | STBY | 高电平使能 |
| 共地 | GND | GND | 必须共地 |

## 2. TB6612FNG 与电机/电源

| 功能 | 引脚 | 连接 |
|---|---|---|
| 电机输出 | A01/A02 | 直流电机两线 |
| 逻辑电源 | VCC | 3.3V |
| 电机电源 | VM | 5V独立电源 |
| 地 | GND | 与STM32、电机电源共地 |

## 3. 限位与按键

| 功能 | STM32 引脚 | 触发方式 |
|---|---|---|
| 左限位 | PA0 | 低电平触发 |
| 右限位 | PA1 | 低电平触发 |
| 开键 | PA4 | 低电平触发 |
| 停键 | PA5 | 低电平触发 |
| 关键 | PA6 | 低电平触发 |

> 限位推荐接入。若暂时不接，系统仍可通过超时保护与软限位兜底。

## 4. 串口链路（网关<->STM32）

| STM32 | USB-TTL/网关 |
|---|---|
| PA9(TX) | RX |
| PA10(RX) | TX |
| GND | GND |

串口参数：`115200 8N1`

## 5. INMP441 与 Orange Pi Zero 3（I2S3）

| INMP441 | Orange Pi Zero 3 | 说明 |
|---|---|---|
| VDD | 3.3V (Pin1/17) | 仅3.3V |
| GND | GND | 共地 |
| SCK | PH6 (Pin23) | BCLK |
| WS | PH7 (Pin19) | LRCLK |
| SD | PH9 (Pin24) | DIN |
| L/R | GND | 左声道 |

## 6. 电机执行器代码映射（对应实现）

- 状态机核心：`firmware/.../Core/Src/curtain_ctrl.c`
- 硬件抽象与UART签名帧：`firmware/.../Core/Src/app_entry.c`
- 结构体与故障码定义：`firmware/.../Core/Inc/curtain_ctrl.h`

关键字段：

- `current_pos` / `target_pos`
- `motion_dir` / `pwm_now` / `pwm_target`
- `travel_ticks_full` / `timeout_ticks`
- `fault_code` / `recoverable`

## 7. 上电前检查

1. VM电源是否独立、容量足够。
2. 所有地是否共地。
3. STBY默认是否可控。
4. 电机机械负载是否卡滞。
5. 串口TX/RX是否交叉连接。
