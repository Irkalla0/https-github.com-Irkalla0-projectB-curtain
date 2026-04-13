# 现在就做（30分钟起步版）

## 步骤A：硬件先通电不接电机

1. 只接 STM32 + ST-Link + 串口（USB-TTL）
2. 下载一个最小串口打印程序
3. 串口看到 `boot ok`

## 步骤B：接 TB6612 与电机空载

1. 接 `AIN1/AIN2/PWMA/STBY` 到 STM32
2. 电机单独 5V 供电，和 STM32 共地
3. 固件里手动切方向和占空比，确认正反转

## 步骤C：接限位与按键

1. 左右限位上拉输入
2. 三个按键上拉输入
3. 串口输出按键/限位变化

## 步骤D：导入本仓库模板

导入 `firmware/stm32_mvp_template`：

- `curtain_ctrl.h/.c`
- `curtain_port.h`
- `example_main_loop.c`

先把 `Port_ReadInputs()` 用真实GPIO读值替换掉，今天就能跑状态机。

## Day1完成判定

- 开键 -> 电机开方向
- 关键 -> 电机关方向
- 停键 -> 停机
- 触发对应限位 -> 自动停机

