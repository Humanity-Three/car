## Example Summary

Empty project using DriverLib.
This example shows a basic empty project using DriverLib with just main file
and SysConfig initialization.

## Peripherals & Pin Assignments

| Peripheral | Pin | Function |
| --- | --- | --- |
| SYSCTL |  |  |
| DEBUGSS | PA20 | Debug Clock |
| DEBUGSS | PA19 | Debug Data In Out |

## BoosterPacks, Board Resources & Jumper Settings

Visit [LP_MSPM0G3507](https://www.ti.com/tool/LP-MSPM0G3507) for LaunchPad information, including user guide and hardware files.

| Pin | Peripheral | Function | LaunchPad Pin | LaunchPad Settings |
| --- | --- | --- | --- | --- |
| PA20 | DEBUGSS | SWCLK | N/A | <ul><li>PA20 is used by SWD during debugging<br><ul><li>`J101 15:16 ON` Connect to XDS-110 SWCLK while debugging<br><li>`J101 15:16 OFF` Disconnect from XDS-110 SWCLK if using pin in application</ul></ul> |
| PA19 | DEBUGSS | SWDIO | N/A | <ul><li>PA19 is used by SWD during debugging<br><ul><li>`J101 13:14 ON` Connect to XDS-110 SWDIO while debugging<br><li>`J101 13:14 OFF` Disconnect from XDS-110 SWDIO if using pin in application</ul></ul> |

### Device Migration Recommendations
This project was developed for a superset device included in the LP_MSPM0G3507 LaunchPad. Please
visit the [CCS User's Guide](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration)
for information about migrating to other MSPM0 devices.

### Low-Power Recommendations
TI recommends to terminate unused pins by setting the corresponding functions to
GPIO and configure the pins to output low or input with internal
pullup/pulldown resistor.

SysConfig allows developers to easily configure unused pins by selecting **Board**→**Configure Unused Pins**.

For more information about jumper configuration to achieve low-power using the
MSPM0 LaunchPad, please visit the [LP-MSPM0G3507 User's Guide](https://www.ti.com/lit/slau873).

## Example Usage

Compile, load and run the example.

---

# 循迹小车硬件连接说明

主控芯片：**MSPM0G3505** (MSPM0G350X)

---

## 1. OLED 显示屏 (0.96寸, I2C 软件模拟)

| OLED 引脚 | MCU 引脚 | 端口 |
|:---------|:---------|:----|
| SCL      | PA28     | GPIOA |
| SDA      | PA31     | GPIOA |
| VCC      | 3.3V     | - |
| GND      | GND      | - |

> OLED 模块需有板载 4.7kΩ 上拉电阻，如无则需在 SCL/SDA 上外接。

---

## 2. MPU6050 六轴陀螺仪 (硬件 I2C1)

| MPU6050 引脚 | MCU 引脚 | 端口 |
|:------------|:---------|:----|
| SCL         | PB2      | GPIOB |
| SDA         | PB3      | GPIOB |
| VCC         | 3.3V     | - |
| GND         | GND      | - |

> I2C 地址：0x68（AD0 引脚悬空或接 GND）

---

## 3. 电机驱动 — TB6612

| TB6612 引脚 | MCU 引脚 | 端口 |
|:-----------|:---------|:----|
| PWMA       | PA12     | GPIOA (TIMG0_CCP0) |
| PWMB       | PA13     | GPIOA (TIMG0_CCP1) |
| AIN1       | PB16     | GPIOB |
| AIN2       | PB0      | GPIOB |
| BIN1       | PB7      | GPIOB |
| BIN2       | PB6      | GPIOB |
| STBY       | PB12     | GPIOB |
| VM         | 电池电压 (7.4V~12V) | - |
| VCC        | 3.3V / 5V | - |
| GND        | GND      | - |

> STBY 拉高使能电机驱动芯片。

---

## 4. 编码器电机

| 编码器引脚 | MCU 引脚 | 端口 |
|:----------|:---------|:----|
| 电机A-CHA  | PA17     | GPIOA (中断) |
| 电机A-CHB  | PA24     | GPIOA (中断) |
| 电机B-CHA  | PB18     | GPIOB (中断) |
| 电机B-CHB  | PB19     | GPIOB (中断) |
| 电机A电源  | PWM 输出 (PA12) | - |
| 电机B电源  | PWM 输出 (PA13) | - |

---

## 5. 灰度传感器 (8路)

| 灰度传感器 | MCU 引脚 | 端口 |
|:----------|:---------|:----|
| AD0       | PB5      | GPIOB (输出) |
| AD1       | PA8      | GPIOA (输出) |
| AD2       | PA9      | GPIOA (输出) |
| DO (数据) | PB4      | GPIOB (输入) |
| VCC       | 3.3V~5V | - |
| GND       | GND      | - |

> AD0/AD1/AD2 用于通道选择（3 位地址线，选通 8 路中的 1 路）。

---

## 6. 按键

| 按键 | MCU 引脚 | 端口 |
|:----|:---------|:----|
| S1   | PA14     | GPIOA (内部上拉) |
| S2   | PB21     | GPIOB (内部上拉) |

> S1 — 启动/停止；S2 — 切换模式（长按清零 Yaw 角）。

---

## 7. 板载 LED

| LED | MCU 引脚 | 端口 |
|:---|:---------|:----|
| LED | PB13     | GPIOB |

---

## 8. 串口 (UART0)

| UART 引脚 | MCU 引脚 | 端口 |
|:---------|:---------|:----|
| TX       | PA10     | GPIOA |
| RX       | PA11     | GPIOA |

> 波特率：115200

---

## 9. 电源

| 模块 | 电压 |
|:----|:----|
| 主控板供电 | 3.3V（从 TB6612 VCC 取电或独立 LDO） |
| 电机供电 | 7.4V~12V（电池 → TB6612 VM） |
| OLED | 3.3V |
| MPU6050 | 3.3V |
| 灰度传感器 | 3.3V~5V |
