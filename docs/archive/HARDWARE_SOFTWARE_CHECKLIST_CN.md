# 从零开始清单（预算500内优先版）

> 预算随平台波动，以下是常见低预算组合。

## 1. 必买硬件

- STM32 开发板 x1
- TB6612FNG 模块 x1
- 直流减速电机（两线）x1
- 5V/2A 电机电源 x1
- Orange Pi Zero 3（1GB）x1
- INMP441 麦克风 x1
- MicroSD 16GB+ x1
- 读卡器 x1
- CH340 USB-TTL x1
- 杜邦线/面包板/支架/联轴器若干

## 2. 必装软件

- PC：STM32CubeMX、STM32CubeIDE、Git、串口工具
- Orange Pi：Armbian、python3、pip、alsa-utils
- Python库：`requests` `sounddevice` `vosk` `pyyaml`

## 3. 最小开工条件

- STM32 烧录成功
- 电机开/关/停跑通
- Orange Pi 能识别 INMP441（`arecord -l`）
- 网关 API 可用（`/target /stop /status /faults`）
