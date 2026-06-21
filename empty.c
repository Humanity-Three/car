/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti/devices/msp/m0p/mspm0g350x.h"
#include "ti_msp_dl_config.h"
#include "stdio.h"
#include "string.h"
#include "control.h"
#include "OLED.h"
#include "MPU6050.h"
#include "Grayscale_Sensor/grayscale_sensor.h"
#include "Delay.h"

extern float Yaw;
extern float Pitch;
extern float Roll;
extern uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];

// 灰度传感器宏定义
#define P1			g_sensor_data[0]
#define P2			g_sensor_data[1]
#define P3			g_sensor_data[2]
#define P4			g_sensor_data[3]
#define P5			g_sensor_data[4]
#define P6			g_sensor_data[5]
#define P7			g_sensor_data[6]
#define P8			g_sensor_data[7]

extern float tim;
extern int timbegin;
char str[64];
uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];  // 灰度传感器数据数组定义


int main(void)
{
    uint8_t a=5;
    SYSCFG_DL_init();
 
    NVIC_EnableIRQ(TIMER_Encoder_Read_INST_INT_IRQN);                  //开启定时器
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(GPIO_EncoderA_INT_IRQN);
    NVIC_EnableIRQ(GPIO_EncoderB_INT_IRQN);
    DL_Timer_startCounter(TIMER_Encoder_Read_INST);
    DL_Timer_startCounter(TIMER_0_INST);
    DL_Timer_startCounter(PWM_0_INST);                 //pwm 定时器初始化
    Alert_UpdateOutput();
    OLED_Init();

    if (!MPU6050_Init()) {
        OLED_ShowString(0, 0, "MPU6050 FAIL!", OLED_8X16);
        OLED_Update();
        while(1);
    }
    Grayscale_Sensor_Init();                          //初始化灰度传感器
    
    int mode=0;
    int begin=0;
    //DL_GPIO_clearPins(GPIO_STBY_PORT,GPIO_STBY_PIN_STBY_PIN);
    DL_GPIO_setPins(GPIO_STBY_PORT,GPIO_STBY_PIN_STBY_PIN);
    uint8_t blink = 0;
    while (1) 
    {
        blink = !blink;                          //每循环翻转一次
        OLED_Clear();
        if(!DL_GPIO_readPins(GPIO_Key_PIN_S2_PORT, GPIO_Key_PIN_S2_PIN))
        {
            Delay_ms(10);
            if(!DL_GPIO_readPins(GPIO_Key_PIN_S2_PORT, GPIO_Key_PIN_S2_PIN))
            {
                // 等待松开，同时计时区分短按/长按
                int key_count = 0;
                while(!DL_GPIO_readPins(GPIO_Key_PIN_S2_PORT, GPIO_Key_PIN_S2_PIN) && key_count < 200)
                {
                    Delay_ms(10);
                    key_count++;
                }
                if(key_count < 190) // 短按（< 1.9 秒）→ 切换模式
                {
                    mode = (mode + 1) % 4;
                }
                else                // 长按（≥ 1.9 秒）→ Yaw 清零
                {
                    Yaw = 0;
                }
            }
            while(!DL_GPIO_readPins(GPIO_Key_PIN_S2_PORT, GPIO_Key_PIN_S2_PIN));
        }
        else if(!DL_GPIO_readPins(GPIO_Key_PIN_S1_PORT, GPIO_Key_PIN_S1_PIN))
        {
            Delay_ms(10);
            if(!DL_GPIO_readPins(GPIO_Key_PIN_S1_PORT, GPIO_Key_PIN_S1_PIN))
            {
                Delay_ms(1000);
                DL_GPIO_setPins(GPIO_STBY_PORT,GPIO_STBY_PIN_STBY_PIN);
                timbegin=1;
                begin=1;
                Alert_Trigger();
            }
            while(!DL_GPIO_readPins(GPIO_Key_PIN_S1_PORT, GPIO_Key_PIN_S1_PIN));
        }
        
        if (mode==0)
        {
            Delay_ms(2);                       //等待电机噪声稳定后再读取传感器
            Grayscale_Sensor_Read_All(g_sensor_data);   //读取灰度传感器数据
            MPU6050_Read_Data();
            MPU6050_Get_Angle();
            OLED_ShowString(0, 0, "AB",OLED_8X16);
            sprintf(str,"time:%.2f",tim);
            OLED_ShowString(0, 16,str,OLED_8X16);
            OLED_Printf(0, 32, OLED_8X16, "Yaw:%5.2f",Yaw);
            sprintf(str,"G:%d%d%d%d%d%d%d%d",P1,P2,P3,P4,P5,P6,P7,P8);
            OLED_ShowString(0, 48, str,OLED_8X16);
            OLED_ShowChar(120, 0, blink ? '*' : ' ', OLED_8X16);  //运行指示：闪烁星号
            OLED_Update();
            if (begin==1)
            {
                Control_AB();
            }
        }
        else if(mode==1)
        {
            Delay_ms(2);                       //等待电机噪声稳定后再读取传感器
            Grayscale_Sensor_Read_All(g_sensor_data);   //读取灰度传感器数据
            MPU6050_Read_Data();
            MPU6050_Get_Angle();
            OLED_ShowString(0, 0, "ABCDA",OLED_8X16);
            sprintf(str,"time:%.2f",tim);
            OLED_ShowString(0, 16,str,OLED_8X16);
            OLED_Printf(0, 32, OLED_8X16, "Yaw:%5.2f",Yaw);
            sprintf(str,"G:%d%d%d%d%d%d%d%d",P1,P2,P3,P4,P5,P6,P7,P8);
            OLED_ShowString(0, 48, str,OLED_8X16);
            OLED_ShowChar(120, 0, blink ? '*' : ' ', OLED_8X16);  //运行指示：闪烁星号
            OLED_Update();
            if (begin==1)
            {
                Control_ABCDA();
            }
        }
        else if(mode==2)
        {
            Delay_ms(2);                       //等待电机噪声稳定后再读取传感器
            Grayscale_Sensor_Read_All(g_sensor_data);   //读取灰度传感器数据
            MPU6050_Read_Data();
            MPU6050_Get_Angle();
            OLED_ShowString(0, 0, "ACBDA",OLED_8X16);
            sprintf(str,"time:%.2f",tim);
            OLED_ShowString(0, 16,str,OLED_8X16);
            OLED_Printf(0, 32, OLED_8X16, "Yaw:%5.2f",Yaw);
            sprintf(str,"G:%d%d%d%d%d%d%d%d",P1,P2,P3,P4,P5,P6,P7,P8);
            OLED_ShowString(0, 48, str,OLED_8X16);
            OLED_ShowChar(120, 0, blink ? '*' : ' ', OLED_8X16);  //运行指示：闪烁星号
            OLED_Update();
            if (begin==1)
            {
                Control_ACBDA();
            }
        }
        else if(mode==3)
        {
            Delay_ms(2);                       //等待电机噪声稳定后再读取传感器
            Grayscale_Sensor_Read_All(g_sensor_data);   //读取灰度传感器数据
            MPU6050_Read_Data();
            MPU6050_Get_Angle();
            OLED_ShowString(0, 0, "ACBDAx4",OLED_8X16);
            sprintf(str,"time:%.2f",tim);
            OLED_ShowString(0, 16,str,OLED_8X16);
            OLED_Printf(0, 32, OLED_8X16, "Yaw:%5.2f",Yaw);
            sprintf(str,"G:%d%d%d%d%d%d%d%d",P1,P2,P3,P4,P5,P6,P7,P8);
            OLED_ShowString(0, 48, str,OLED_8X16);
            OLED_ShowChar(120, 0, blink ? '*' : ' ', OLED_8X16);  //运行指示：闪烁星号
            OLED_Update();
            if (begin==1)
            {
                Control_ACBDAx4();
            }
        }
        
    }
}


// /*********************串口重定向********************/
// int fputc(int c, FILE* stream)
// {
// 	DL_UART_Main_transmitDataBlocking(UART_0_INST, c);
//     return c;
// }

// int fputs(const char* restrict s, FILE* restrict stream)
// {
//     uint16_t i, len;
//     len = strlen(s);
//     for(i=0; i<len; i++)
//     {
//         DL_UART_Main_transmitDataBlocking(UART_0_INST, s[i]);           //发送完成后执行后续程序    串口0
//     }
//     return len;
// }

// int puts(const char *_ptr)
// {
//     int count = fputs(_ptr ,stdout);
//     count += fputs("\n",stdout);
//     return count;
// }
// /**********************************************************************************/
