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

## 2. MPU6050 六轴陀螺仪 (软件模拟 I2C)

| MPU6050 引脚 | MCU 引脚 | 端口 |
|:------------|:---------|:----|
| SCL         | **PA1**  | GPIOA (软件 I2C) |
| SDA         | **PA0**  | GPIOA (软件 I2C) |
| VCC         | 3.3V     | - |
| GND         | GND      | - |

> - I2C 地址：0x68（AD0 引脚悬空或接 GND）
> - 使用 GPIO 软件模拟 I2C，非硬件 I2C 外设

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

> - STBY 拉高使能电机驱动芯片
> - 电机正反转由 AIN1/AIN2（左）和 BIN1/BIN2（右）电平组合控制

### TB6612 ←→ 电机

| TB6612 引脚 | 电机接线 |
|:-----------|:---------|
| **A01** / **A02** | 左电机 M+ / M- |
| **B01** / **B02** | 右电机 M+ / M- |
| **VM** | 电池正极 (7.4V~12V) |
| **GND** (与 MCU 共地) | 电池负极 |

> 如果电机转向与预期相反，将该路的两根电机线互换即可。

---

## 4. 编码器电机

### 编码器信号 ←→ MCU

| 编码器引脚 | MCU 引脚 | 端口 | 说明 |
|:----------|:---------|:----|:-----|
| 左电机-CHA  | **PA17** | GPIOA | 中断输入，测速 |
| 左电机-CHB  | **PA24** | GPIOA | 中断输入，判向 |
| 右电机-CHA  | **PB18** | GPIOB | 中断输入，测速 |
| 右电机-CHB  | **PB19** | GPIOB | 中断输入，判向 |
| 编码器 VCC  | 3.3V~5V | - | 视编码器型号而定 |
| 编码器 GND  | GND | - | 共地 |

> - 编码器由电机上的 PWM 线 (PA12/PA13) 供电，VCC/GND 仅给编码器逻辑供电
> - 如果编码器计数方向与实际转向相反，交换该路的 CHA 和 CHB 即可

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

| 模块 | 电压 | 说明 |
|:----|:----|:-----|
| 主控板供电 | 3.3V | 从 TB6612 VCC 取电，或独立 3.3V LDO |
| 电机供电 | 7.4V~12V | 电池正极 → TB6612 VM，负极与 MCU GND 共地 |
| OLED | 3.3V | 接开发板 3.3V 或 TB6612 VCC |
| MPU6050 | 3.3V | 接开发板 3.3V |
| 灰度传感器 | 3.3V~5V | 接开发板 3.3V 或 TB6612 VCC |
| 编码器逻辑供电 | 3.3V~5V | 视编码器型号，建议先接 3.3V |

> **关键**：电池负极、TB6612 GND、MCU GND 必须**共地**，否则电机和编码器不工作。

---

## 10. 整体接线总览

```
                         ┌─────────────────────────────┐
                         │         MCU 开发板            │
                         │                             │
    ┌────────────────────┤  PA0 ── MPU6050 SDA         │
    │                    │  PA1 ── MPU6050 SCL          │
    │    TB6612          │  PA8 ── 灰度 AD1            │
    │                    │  PA9 ── 灰度 AD2            │
    │    PWMA ──── PA12  │  PA10 ── UART TX            │
    │    PWMB ──── PA13  │  PA11 ── UART RX            │
    │    AIN1 ──── PB16  │  PA12 ── TB6612 PWMA + 左电机 PWM         │
    │    AIN2 ──── PB0   │  PA13 ── TB6612 PWMB + 右电机 PWM         │
    │    BIN1 ──── PB7   │  PA14 ── 按键 S1            │
    │    BIN2 ──── PB6   │  PA17 ── 左编码器 CHA         │
    │    STBY ─── PB12   │  PA24 ── 左编码器 CHB         │
    │    VCC  ─── 3.3V   │  PA28 ── OLED SCL           │
    │    VM   ─── 电池+  │  PA31 ── OLED SDA           │
    │    GND  ─── 电池-  │                             │
    │                    │  PB4 ── 灰度 DO             │
    │    A01 ── 左电机M+ │  PB5 ── 灰度 AD0            │
    │    A02 ── 左电机M- │  PB6 ── TB6612 BIN2         │
    │    B01 ── 右电机M+ │  PB7 ── TB6612 BIN1         │
    │    B02 ── 右电机M- │  PB12 ── TB6612 STBY        │
    │                    │  PB13 ── 板载 LED           │
    │   左编码器 CHA ─ PA17│  PB16 ── TB6612 AIN1       │
    │   左编码器 CHB ─ PA24│  PB18 ── 右编码器 CHA      │
    │   右编码器 CHA ─ PB18│  PB19 ── 右编码器 CHB      │
    │   右编码器 CHB ─ PB19│  PB21 ── 按键 S2          │
    │                    │  PB0  ── TB6612 AIN2         │
    │                    │                             │
    │                    │  GND ─── 电池负极（共地）    │
    └────────────────────┤  3.3V ── TB6612 VCC         │
                         └─────────────────────────────┘
```
