//Grayscale Sensor / MSPM0
// 	 	 5V 	 	 	 5V
// 	 	 GND 	 	 	 GND
// 	 	 AD0 	 	 	 PA14
// 	 	 AD1 	 	 	 PA15
// 	 	 AD2 	 	 	 PA16
// 	 	 OUT 	 	 	 PA17
#include "ti_msp_dl_config.h"
#include "Delay.h"
#include "grayscale_sensor.h"

// 外部声明 g_sensor_data 数组（在 empty.c 中定义）
extern uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];

// 通道选择编码表 (AD2, AD1, AD0) -> 通道 0-7
// 通道 0: AD2=0, AD1=0, AD0=0
// 通道 1: AD2=0, AD1=0, AD0=1
// 通道 2: AD2=0, AD1=1, AD0=0
// 通道 3: AD2=0, AD1=1, AD0=1
// 通道 4: AD2=1, AD1=0, AD0=0
// 通道 5: AD2=1, AD1=0, AD0=1
// 通道 6: AD2=1, AD1=1, AD0=0
// 通道 7: AD2=1, AD1=1, AD0=1

/**
 * @brief 初始化灰度传感器 GPIO
 */
void Grayscale_Sensor_Init(void)
{
    // 初始化通道选择引脚为输出模式
    // 注意：需要在 SysConfig 中配置 GrayS_PORT 的对应引脚为输出模式
    SENSOR_AD0_WRITE(0);
    SENSOR_AD1_WRITE(0);
    SENSOR_AD2_WRITE(0);
    
    // 短暂延时等待稳定
    Delay_ms(10);
}

/**
 * @brief 读取单个通道的灰度传感器值
 * @param channel 通道号 (0-7)
 * @return 传感器值 (0 或 1，1 表示检测到黑色)
 */
uint16_t Grayscale_Sensor_Read_Single(uint8_t channel)
{
    uint8_t ad2, ad1, ad0;
    uint16_t sensor_value;
    
    // 根据通道号设置 AD2, AD1, AD0
    ad0 = (channel >> 0) & 0x01;
    ad1 = (channel >> 1) & 0x01;
    ad2 = (channel >> 2) & 0x01;
    
    // 设置通道选择引脚
    SENSOR_AD0_WRITE(ad0);
    SENSOR_AD1_WRITE(ad1);
    SENSOR_AD2_WRITE(ad2);
    
    // 短暂延时等待信号稳定
    Delay_ms(1);
    
    // 读取 OUT 引脚状态
    sensor_value = SENSOR_OUT_READ();
    
    return sensor_value;
}

/**
 * @brief 读取所有 8 个通道的灰度传感器值
 * @param sensor_values 存储结果的数组，长度至少为 8
 */
void Grayscale_Sensor_Read_All(uint16_t* sensor_values)
{
    uint8_t i;
    
    for(i = 0; i < GRAYSCALE_SENSOR_CHANNELS; i++)
    {
        sensor_values[i] = Grayscale_Sensor_Read_Single(i);
    }
}
