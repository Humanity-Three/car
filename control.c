#include "control.h"
#include "stdio.h"
#include "string.h"
#include "Grayscale_Sensor/grayscale_sensor.h"

// 声明外部数组（在 empty.c 中定义）
extern uint16_t g_sensor_data[GRAYSCALE_SENSOR_CHANNELS];

#define AIN11	DL_GPIO_setPins(GPIO_IN_PORT,GPIO_IN_PIN_AIN1_PIN)
#define AIN10	DL_GPIO_clearPins(GPIO_IN_PORT,GPIO_IN_PIN_AIN1_PIN)
#define AIN21	DL_GPIO_setPins(GPIO_IN_PORT,GPIO_IN_PIN_AIN2_PIN)
#define AIN20	DL_GPIO_clearPins(GPIO_IN_PORT,GPIO_IN_PIN_AIN2_PIN)

#define BIN11	DL_GPIO_setPins(GPIO_IN_PORT,GPIO_IN_PIN_BIN1_PIN)
#define BIN10	DL_GPIO_clearPins(GPIO_IN_PORT,GPIO_IN_PIN_BIN1_PIN)
#define BIN21	DL_GPIO_setPins(GPIO_IN_PORT,GPIO_IN_PIN_BIN2_PIN)
#define BIN20	DL_GPIO_clearPins(GPIO_IN_PORT,GPIO_IN_PIN_BIN2_PIN)

// 灰度传感器宏定义 (使用新驱动)
// g_sensor_data 数组在 empty.c 中定义
#define P1			g_sensor_data[0]
#define P2			g_sensor_data[1]
#define P3			g_sensor_data[2]
#define P4			g_sensor_data[3]
#define P5			g_sensor_data[4]
#define P6			g_sensor_data[5]
#define P7			g_sensor_data[6]
#define P8			g_sensor_data[7]

volatile uint32_t gpioA,gpioB;
volatile int32_t gEncoderCount_left = 0, gEncoderVal_left = 0;          //左轮编码器计数值；左轮编码器记录值，50ms 定时中断输出
volatile int32_t gEncoderCount_right = 0, gEncoderVal_right = 0;        //右轮编码器计数值；右轮编码器记录值，50ms 定时中断输出
volatile int32_t gEncoderMileage_left = 0, gEncoderMileage_right = 0;   //编码器累计里程计数
volatile uint32_t g_sys_tick_10ms = 0;                                  //10ms 系统滴答，用于 MPU6050 实际 DT
extern float Yaw;                                                       //陀螺仪偏转角返回值
extern float Pitch;                                                     //陀螺仪俯仰角返回值
extern float Roll;                                                      //陀螺仪横滚角返回值

#define  Limit		200						//PWM波限幅，百分比制
#define  ACBDA_DIAGONAL_TICKS  4100        //0.8m 和 1.0m 直角边斜边约 1.28m，对应编码器计数需实测标定
#define  ACBDA_X4_LAP2_YAW_CORRECTION  1.5f //ACBDAx4 第 2 圈离开黑线后的转向角度修正值
#define  ACBDA_X4_LAP3_YAW_CORRECTION  3.0f //ACBDAx4 第 3 圈离开黑线后的转向角度修正值
#define  ACBDA_X4_LAP4_YAW_CORRECTION  4.5f //ACBDAx4 第 4 圈离开黑线后的转向角度修正值

//速度环PID
#define   Kp1   			0.9
#define   Ki1     		    0              //经测试不需要I环和D环
#define   Kd1  				0

float CurrentA, CurrentB;			//编码器测得当前速度
float targetA=0, targetB=0;			//当前目标速度
float Speed_diff; 					//当前差速

int flag=1,n=1,whiteflag=0,whiteflag1=0,whiteflag2=0;         //白色区域计数标志位；白色区域计数值；陀螺仪模式标志位
int timebegin=0,timenum=0;          //陀螺仪模式启动延时标志位；延时计数值
int timebegin1=0,timenum1=0;          //陀螺仪模式启动延时标志位；延时计数值
int timebegin2=0,timenum2=0;          //陀螺仪模式启动延时标志位；延时计数值
int ledbegin=0,lednum=0,ledflag=0,ledflag1=0,ledflag2=0;        //声光模块延时标志位；延时计数值；声光模块启动标志位
float error=180;                                                //两个方向的偏转
int m=0;                                                        //模式切换计数值，用以判断是否跑完全程
int a=0;                                                        //AB模式停止标志位
int pwmstart=1;
float tim=0;
int timbegin=0,timnum=0;
static float start_yaw_acbda = 0.0f;
static int start_yaw_ready_acbda = 0;
static int acbda_first_line_seen = 0;

static float Normalize_Yaw_Error(float current, float target)
{
    float diff = current - target;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return diff;
}

static int xunji_acbda_fast(void)
{
    if(P4!=0)
	{
		return -18;
	}
	else if(P5!=0)
	{
		return 18;
	}
    else if(P3!=0)
	{
		return -32;
	}
	else if(P6!=0)
	{
		return 32;
	}
	else if(P2!=0)
	{
		return -52;
	}
	else if(P7!=0)
	{
		return 52;
	}
	else if(P1!=0)
	{
		return -90;
	}
	else if(P8!=0)
	{
		return 90;
	}
	return 0;
}

void Control_AB(void)                       //模式一
{
    float Speed_Middle=70;				//中值速度
    int Motor_Left, Motor_Right;            //电机赋值
    float bias;
    bias = Yaw;
    if(bias < 2.0f && bias > -2.0f) bias = 0.0f;    // 死区：忽略微小漂移

    if ((P1!=0 || P2!=0 || P3!=0 || P4!=0 || P5!=0 || P6!=0 || P7!=0 || P8!=0)&&a==0)
    {
        // DL_GPIO_clearPins(GPIO_STBY_PORT,GPIO_STBY_PIN_STBY_PIN);
        timbegin=0;
        pwmstart=0;
        ledbegin=1;
        a=1;
    }

    if (ledflag==1)
    {
        DL_GPIO_setPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);
    }
    else DL_GPIO_clearPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);

    targetA = Speed_Middle+bias;
	targetB = Speed_Middle-bias;
    CurrentA = (float)gEncoderVal_left/3; //left
	CurrentB = (float)gEncoderVal_right/3; //right
	Motor_Left  = (int)PWM_Limit(PID_A(CurrentA,targetA),Limit, -Limit);
	Motor_Right = (int)PWM_Limit(PID_B(CurrentB,targetB),Limit, -Limit);		//PWM限幅

    if(pwmstart==1) Set_Pwm(Motor_Left, Motor_Right);
    else if(pwmstart==0) Set_Pwm(1, 1);
}
	
void Control_ABCDA(void)
{
    float Speed_Middle=70;				//中值速度
	int Motor_Left, Motor_Right;
    float bias;
    int black_count;
    static int last_whiteflag = 0;
    static int white_count = 0;
    static int black_seen_count = 0;
    static float white_yaw_target = 0.0f;
    static float start_yaw = 0.0f;
    static int start_yaw_ready = 0;
    static int abcda_finished = 0;
    static int black_arc_ticks = 0;
    static int da_arc_ready_to_stop = 0;
    static int ready_for_next_black = 1;

    if (ledflag==1)
    {
        DL_GPIO_setPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);
    }
    else DL_GPIO_clearPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);

    if (abcda_finished == 1)
    {
        Set_Pwm(1, 1);
        timbegin = 0;
        return;
    }

    if (start_yaw_ready == 0)
    {
        start_yaw = Yaw;
        start_yaw_ready = 1;
    }

    black_count = P1 + P2 + P3 + P4 + P5 + P6 + P7 + P8;    

    if (black_count == 0)
    {
        whiteflag = 1;                  // 全白 → 进入白色直行模式
    }
    else
    {
        whiteflag = 0;                  // 看到黑线 → 切回寻迹
    }
    
    if (black_count == 0)
    {
        white_count++;
        black_seen_count = 0;
        if (white_count < 3)
        {
            whiteflag = last_whiteflag;
        }
        if (white_count >= 20)
        {
            ready_for_next_black = 1;
        }
    }
    else
    {
        black_seen_count++;
        white_count = 0;
        if (black_seen_count < 2)
        {
            whiteflag = last_whiteflag;
        }
    }

    if (whiteflag == 1 && last_whiteflag == 0 && da_arc_ready_to_stop == 1)
    {
        abcda_finished = 1;
        last_whiteflag = whiteflag;
        Set_Pwm(1, 1);
        timbegin = 0;
        return;
    }

    if (whiteflag==1) 
    {
        /* ── 白色直道（A→B 或 C→D）：陀螺仪修正方向直行 ── */
        if (last_whiteflag == 0)
        {
            if (n % 2 == 0)
            {
                white_yaw_target = 180.0f - start_yaw;
            }
            else
            {
                white_yaw_target = start_yaw;
            }
        }
        bias = Normalize_Yaw_Error(Yaw, white_yaw_target);
        if (bias > 35.0f) bias = 35.0f;
        if (bias < -35.0f) bias = -35.0f;
        flag=0;
    }
    else if(whiteflag==0)
    {
        /* ── 黑色弧线（B→C 或 D→A）：寻迹循线 ── */
        if(flag==0)
        {
            flag=1;
            if (ready_for_next_black == 1)
            {
                n=n+1;
                ready_for_next_black = 0;
                black_arc_ticks = 0;
            }
        }
        black_arc_ticks++;
        if (n >= 3 && black_arc_ticks >= 20)
        {
            da_arc_ready_to_stop = 1;
        }
        bias = xunji();
    }

    last_whiteflag = whiteflag;

    targetA = Speed_Middle+bias;
	targetB = Speed_Middle-bias;
    CurrentA = (float)gEncoderVal_left/3; //left
	CurrentB = (float)gEncoderVal_right/3; //right
	Motor_Left  = (int)PWM_Limit(PID_A(CurrentA,targetA),Limit, -Limit);
	Motor_Right = (int)PWM_Limit(PID_B(CurrentB,targetB),Limit, -Limit);		//PWM限幅

	Set_Pwm(Motor_Left, Motor_Right);
}

void Control_ACBDA(void)
{
    float Speed_Middle=60;				//中值速度
    int Motor_Left, Motor_Right;
    float bias;
    int black_count;
    static int last_whiteflag_acbda = 0;
    static int white_count_acbda = 0;
    static int black_seen_count_acbda = 0;
    static int ready_for_next_black_acbda = 1;
    static int b_yaw_aligned_acbda = 0;
    static int white_phase_acbda = 0;
    static int32_t white_start_mileage_acbda = 0;
    static int acbda_finished = 0;
    int32_t current_mileage;
    int32_t white_distance;

    if (ledflag==1) DL_GPIO_setPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);
    else DL_GPIO_clearPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);

    if (acbda_finished == 1)
    {
        Set_Pwm(1, 1);
        timbegin = 0;
        return;
    }



    if (start_yaw_ready_acbda == 0)
    {
        start_yaw_acbda = Yaw;
        start_yaw_ready_acbda = 1;
        n = 1;
        m = 0;
        flag = 1;
        whiteflag1 = 0;
        ledflag1 = 0;
        ledflag2 = 0;
        timebegin1 = 0;
        timenum1 = 0;
        acbda_first_line_seen = 0;
        last_whiteflag_acbda = 0;
        white_count_acbda = 0;
        black_seen_count_acbda = 0;
        ready_for_next_black_acbda = 1;
        b_yaw_aligned_acbda = 0;
        white_phase_acbda = 0;
        white_start_mileage_acbda = 0;
        acbda_finished = 0;
    }

    current_mileage = (gEncoderMileage_left + gEncoderMileage_right) / 2;
    black_count = P1 + P2 + P3 + P4 + P5 + P6 + P7 + P8;

    if (black_count == 0)
    {
        whiteflag1 = 1;
    }
    else
    {
        whiteflag1 = 0;
        if (last_whiteflag_acbda == 1 && white_phase_acbda < 2)
        {
            whiteflag1 = 1;
        }
    }

    if (black_count == 0)
    {
        white_count_acbda++;
        black_seen_count_acbda = 0;
        if (white_count_acbda < 16)
        {
            whiteflag1 = last_whiteflag_acbda;
        }
        if (white_count_acbda >= 20)
        {
            ready_for_next_black_acbda = 1;
        }
    }
    else
    {
        black_seen_count_acbda++;
        white_count_acbda = 0;
    }

    if (whiteflag1==1) 
    {
        if (n >= 3 && last_whiteflag_acbda == 0)
        {
            acbda_finished = 1;
            Set_Pwm(1, 1);
            timbegin = 0;
            return;
        }

        ledflag2=0;
        if(ledflag1==0)
        {
            ledflag1=1;
            ledbegin=1;
            m++;
            white_phase_acbda = 0;
            white_start_mileage_acbda = current_mileage;
            b_yaw_aligned_acbda = 0;
        }

        white_distance = current_mileage - white_start_mileage_acbda;

        if (white_phase_acbda == 0)
        {
            if (n%2==1)
            {
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda - 38.6f);
            }
            else
            {
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda + 180.0f + 39.4f);
            }
            if (white_distance >= ACBDA_DIAGONAL_TICKS)
            {
                white_phase_acbda = 1;
            }
        }
        else if (white_phase_acbda == 1)
        {
            if (n%2==1)
            {
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda);
            }
            else
            {
                bias = Normalize_Yaw_Error(Yaw, 180.0f - start_yaw_acbda);
            }
            if (bias < 5.0f && bias > -5.0f)
            {
                white_phase_acbda = 2;
            }
        }
        else
        {
            bias = 0.0f;
        }
        if (bias > 24.0f) bias = 24.0f;
        if (bias < -24.0f) bias = -24.0f;
        flag=0;
    }
    else if(whiteflag1==0)
    {
        ledflag1=0;
        if(ledflag2==0)
        {
            ledflag2=1;
            ledbegin=1;
            m++;
        }

        if (acbda_first_line_seen == 0)
        {
            acbda_first_line_seen = 1;
            flag = 1;
        }
        if(flag==0)
        {
            flag=1;
            if (ready_for_next_black_acbda == 1)
            {
                n=n+1;
                ready_for_next_black_acbda = 0;
                b_yaw_aligned_acbda = 0;
                white_phase_acbda = 0;
            }
        }

        bias = xunji();
        
        whiteflag1=0;
    }

    last_whiteflag_acbda = whiteflag1;


    targetA = Speed_Middle+bias;
	targetB = Speed_Middle-bias;
    CurrentA = (float)gEncoderVal_left/3; //left
	CurrentB = (float)gEncoderVal_right/3; //right
	Motor_Left  = (int)PWM_Limit(PID_A(CurrentA,targetA),Limit, -Limit);
	Motor_Right = (int)PWM_Limit(PID_B(CurrentB,targetB),Limit, -Limit);		//PWM限幅

	Set_Pwm(Motor_Left, Motor_Right);
}

void Control_ACBDAx4(void)
{
    float Speed_Middle=60;				// 中值速度
    int Motor_Left, Motor_Right;
    float bias;
    int black_count;
    static int last_whiteflag_x4 = 0;
    static int white_count_x4 = 0;
    static int black_seen_count_x4 = 0;
    static int ready_for_next_black_x4 = 1;
    static int white_phase_x4 = 0;
    static int32_t white_start_mileage_x4 = 0;
    static int x4_finished = 0;
    static int x4_initialized = 0;
    int32_t current_mileage;
    int32_t white_distance;
    int lap_index;
    float lap_yaw_correction;

    if (ledflag==1) DL_GPIO_setPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);
    else DL_GPIO_clearPins(GPIO_LED_PORT,GPIO_LED_PIN_LED_PIN);

    if (x4_finished == 1)
    {
        Set_Pwm(1, 1);
        timbegin = 0;
        return;
    }

    if (x4_initialized == 0)
    {
        start_yaw_acbda = Yaw;
        start_yaw_ready_acbda = 1;
        n = 1;
        m = 0;
        flag = 1;
        whiteflag1 = 0;
        ledflag1 = 0;
        ledflag2 = 0;
        timebegin1 = 0;
        timenum1 = 0;
        acbda_first_line_seen = 0;
        last_whiteflag_x4 = 0;
        white_count_x4 = 0;
        black_seen_count_x4 = 0;
        ready_for_next_black_x4 = 1;
        white_phase_x4 = 0;
        white_start_mileage_x4 = 0;
        x4_finished = 0;
        x4_initialized = 1;
    }

    current_mileage = (gEncoderMileage_left + gEncoderMileage_right) / 2;
    black_count = P1 + P2 + P3 + P4 + P5 + P6 + P7 + P8;
    lap_index = (n - 1) / 2;
    if (lap_index == 1)
    {
        lap_yaw_correction = ACBDA_X4_LAP2_YAW_CORRECTION;
    }
    else if (lap_index == 2)
    {
        lap_yaw_correction = ACBDA_X4_LAP3_YAW_CORRECTION;
    }
    else if (lap_index == 3)
    {
        lap_yaw_correction = ACBDA_X4_LAP4_YAW_CORRECTION;
    }
    else
    {
        lap_yaw_correction = 0.0f;
    }

    /* ── 模式判断 ── */
    if (black_count == 0)
    {
        whiteflag1 = 1;
    }
    else
    {
        whiteflag1 = 0;
        if (last_whiteflag_x4 == 1 && white_phase_x4 < 2)
        {
            whiteflag1 = 1;
        }
    }

    /* ── 防抖滤波 ── */
    if (black_count == 0)
    {
        white_count_x4++;
        black_seen_count_x4 = 0;
        if (white_count_x4 < 16)
        {
            whiteflag1 = last_whiteflag_x4;
        }
        if (white_count_x4 >= 20)
        {
            ready_for_next_black_x4 = 1;
        }
    }
    else
    {
        black_seen_count_x4++;
        white_count_x4 = 0;
    }

    /* ── 白色区域（直道/斜线） ── */
    if (whiteflag1 == 1)
    {
        /* 完成 4 圈（n 到达 9）后停车，n=9 表示第 4 次 D→A 弧线结束回到 A */
        if (n >= 9 && last_whiteflag_x4 == 0)
        {
            x4_finished = 1;
            Set_Pwm(1, 1);
            timbegin = 0;
            return;
        }

        ledflag2 = 0;
        if (ledflag1 == 0)
        {
            ledflag1 = 1;
            ledbegin = 1;
            m++;
            white_phase_x4 = 0;
            white_start_mileage_x4 = current_mileage;
        }

        white_distance = current_mileage - white_start_mileage_x4;

        if (white_phase_x4 == 0)
        {
            /* 阶段 0：沿对角线行驶到另一侧 */
            if (n % 2 == 1)
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda - 38.6f - lap_yaw_correction);
            else
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda + 180.0f + 39.4f - lap_yaw_correction);

            if (white_distance >= ACBDA_DIAGONAL_TICKS)
                white_phase_x4 = 1;
        }
        else if (white_phase_x4 == 1)
        {
            /* 阶段 1：摆正偏航角对准直行方向 */
            if (n % 2 == 1)
                bias = Normalize_Yaw_Error(Yaw, start_yaw_acbda);
            else
                bias = Normalize_Yaw_Error(Yaw, 180.0f - start_yaw_acbda);

            if (bias < 5.0f && bias > -5.0f)
                white_phase_x4 = 2;
        }
        else
        {
            /* 阶段 2：直行等待黑线 */
            bias = 0.0f;
        }

        if (bias > 24.0f) bias = 24.0f;
        if (bias < -24.0f) bias = -24.0f;
        flag = 0;
    }
    else if (whiteflag1 == 0)
    {
        /* ── 黑色弧线：循迹 ── */
        ledflag1 = 0;
        if (ledflag2 == 0)
        {
            ledflag2 = 1;
            ledbegin = 1;
            m++;
        }

        if (acbda_first_line_seen == 0)
        {
            acbda_first_line_seen = 1;
            flag = 1;
        }
        if (flag == 0)
        {
            flag = 1;
            if (ready_for_next_black_x4 == 1)
            {
                n = n + 1;
                ready_for_next_black_x4 = 0;
                white_phase_x4 = 0;
            }
        }

        bias = xunji();
        whiteflag1 = 0;
    }

    last_whiteflag_x4 = whiteflag1;

    targetA = Speed_Middle + bias;
    targetB = Speed_Middle - bias;
    CurrentA = (float)gEncoderVal_left / 3;
    CurrentB = (float)gEncoderVal_right / 3;
    Motor_Left  = (int)PWM_Limit(PID_A(CurrentA, targetA), Limit, -Limit);
    Motor_Right = (int)PWM_Limit(PID_B(CurrentB, targetB), Limit, -Limit);

    Set_Pwm(Motor_Left, Motor_Right);
}

int xunji(void)			//输出差速，左轮速度为middle+x，右轮速度为middle-x
{
    if(P4!=0)
	{
		return -12;
	}
	else if(P5!=0)
	{
		return 12;
	}
    else if(P3!=0)
	{
		return -22;
	}
	else if(P6!=0)
	{
		return 22;
	}
	else if(P2!=0)
	{
		return -30;
	}
	else if(P7!=0)
	{
		return 30;
	}
	else if(P1!=0)
	{
		return -45;
	}
	else if(P8!=0)
	{
		return 45;
	}
	return 0;
}

int xunji_highspeed_left(void)			//输出差速，左轮速度为middle+x，右轮速度为middle-x
{
    if(P1!=0&&P2!=0)
    {
        return -110;
    }
    else if(P1!=0)
	{
		return -70;
	}
    else if(P2!=0)
	{
		return -40;
	}
    else if(P3!=0)
	{
		return -20;
	}
    else if(P4!=0)
	{
		return -10;
	}
	
	return 0;
}

int xunji_highspeed_right(void)			//输出差速，左轮速度为middle+x，右轮速度为middle-x
{
    if(P8!=0&&P7!=0)
    {
        return 110;
    }
    else if(P8!=0)
	{
		return 70;
	}
    else if(P7!=0)
	{
		return 40;
	}
    else if(P6!=0)
	{
		return 20;
	}
	else if(P5!=0)
	{
		return 10;
	}
	
	return 0;
}

int xunji_lowspeed_left(void)			//输出差速，左轮速度为middle+x，右轮速度为middle-x
{
    if(P1!=0)
	{
		return -45;
	}
    else if(P2!=0)
	{
		return -25;
	}
    else if(P3!=0)
	{
		return -16;
	}
    else if(P4!=0)
	{
		return -10;
	}
	
	return 0;
}

int xunji_lowspeed_right(void)			//输出差速，左轮速度为middle+x，右轮速度为middle-x
{
    if(P8!=0)
	{
		return 45;
	}
    else if(P7!=0)
	{
		return 25;
	}
    else if(P6!=0)
	{
		return 16;
	}
	else if(P5!=0)
	{
		return 10;
	}
	
	return 0;
}

//左/A轮PID
float PID_A(float Encoder,float Target)
{
	static float Bias, Last_bias, Last2_bias, Pwm;
	Bias = Target - Encoder;               																		//计算偏差
	Pwm += Kp1 * (Bias - Last_bias) + Ki1 * Bias + Kd1 * (Bias - 2 * Last_bias + Last2_bias);   									//增量式PI控制器
	Last2_bias = Last_bias;                																		//先保存上上次偏差
	Last_bias = Bias;	                   																		//再保存上一次偏差
	return Pwm;                        				                                        					//返回增量值
}

//右/B轮PID
float PID_B(float Encoder,float Target)
{
	static float Bias, Last_bias, Last2_bias, Pwm;
	Bias = Target-Encoder;               																		//计算偏差
	Pwm += Kp1 * (Bias - Last_bias) + Ki1 * Bias + Kd1 * (Bias - 2 * Last_bias + Last2_bias);   									//增量式PI控制器
	Last2_bias = Last_bias;                																		//先保存上上次偏差
	Last_bias = Bias;	                   																		//再保存上一次偏差
	return Pwm;
}

void Set_Pwm(int motor_left,int motor_right)
{
	if(motor_left > 0)	//前进
	{
        AIN10;
		AIN21;
	}
	else				//后退
	{
		AIN11;
		AIN20;
	}
	//PWMA = myabs(motor_left);
    set_Duty(myabs(motor_left),1);

	if(motor_right > 0) //前进
	{
		BIN10;
		BIN21;
	}
	else				//后退
	{
        BIN11;
		BIN20;
	}
	//PWMB = myabs(motor_right);
    set_Duty(myabs(motor_right),0);
}

void set_Duty(uint8_t duty,uint8_t channel)               //修改pwm波占空比  0~100                 //用timer不要用timerg或timera
{
    uint32_t CompareValue;
    if(duty > 100) duty = 100;                            //钳位到有效范围，防止uint32_t溢出导致PWM锁死
    CompareValue = 2500 - 2500/100*duty;                  //占空比转换

    if(channel == 0)
    {
        DL_Timer_setCaptureCompareValue(PWM_0_INST,CompareValue,DL_TIMER_CC_0_INDEX);           //修改占空比
    }
    else if(channel == 1)
    {
        DL_Timer_setCaptureCompareValue(PWM_0_INST,CompareValue,DL_TIMER_CC_1_INDEX);           
    }
}

void GROUP1_IRQHandler(void)                    //中断服务函数
{
    /**********************编码器读取***********************/
    uint32_t gpioA, gpioB;
    // 最多处理 4 次就退出，防止饿死主循环
    for(int loop = 0; loop < 4; loop++)
    {
        gpioA = DL_GPIO_getEnabledInterruptStatus(GPIOA,GPIO_EncoderA_PIN_0_PIN | GPIO_EncoderA_PIN_1_PIN);
        gpioB = DL_GPIO_getEnabledInterruptStatus(GPIOB,GPIO_EncoderB_PIN_2_PIN | GPIO_EncoderB_PIN_3_PIN);
        if(gpioA == 0 && gpioB == 0) break;  //无待处理的编码器中断，退出

        if((gpioA & GPIO_EncoderA_PIN_0_PIN) == GPIO_EncoderA_PIN_0_PIN)
        {
            //Pin0上升沿
            if(!DL_GPIO_readPins(GPIOA,GPIO_EncoderA_PIN_1_PIN))//P1为高电平
            {
                gEncoderCount_left--;
            }
            else//P1为低电平
            {
                gEncoderCount_left++;
            }
        }
        if((gpioA & GPIO_EncoderA_PIN_1_PIN) == GPIO_EncoderA_PIN_1_PIN)
        {
            //Pin1上升沿
            if(!DL_GPIO_readPins(GPIOA,GPIO_EncoderA_PIN_0_PIN))//P0为高电平
            {
                gEncoderCount_left++;
            }
            else//P1为低电平
            {
                gEncoderCount_left--;
            }
        }

        if((gpioB & GPIO_EncoderB_PIN_2_PIN) != 0)
        {
            if(!DL_GPIO_readPins(GPIOB,GPIO_EncoderB_PIN_3_PIN))
            {
                gEncoderCount_right--;
            }
            else
            {
                gEncoderCount_right++;
            }
        }
        if((gpioB & GPIO_EncoderB_PIN_3_PIN) != 0)
        {
            if(!DL_GPIO_readPins(GPIOB,GPIO_EncoderB_PIN_2_PIN))
            {
                gEncoderCount_right++;
            }
            else
            {
                gEncoderCount_right--;
            }
        }
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_EncoderA_PIN_0_PIN|GPIO_EncoderA_PIN_1_PIN);
        DL_GPIO_clearInterruptStatus(GPIOB, GPIO_EncoderB_PIN_2_PIN|GPIO_EncoderB_PIN_3_PIN);
    }
    /*********************************************************************************************/
}

void TIMER_Encoder_Read_INST_IRQHandler(void)                   //定时中断
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_Encoder_Read_INST)){            //100ms定时，将编码器数据存入val
        case DL_TIMER_IIDX_ZERO:
            gEncoderVal_left = gEncoderCount_left;                              //读取左轮编码器数据
            gEncoderMileage_left += (gEncoderVal_left < 0) ? -gEncoderVal_left : gEncoderVal_left;
            gEncoderCount_left = 0;
            gEncoderVal_right = gEncoderCount_right;                            //读取右轮编码器数据
            gEncoderMileage_right += (gEncoderVal_right < 0) ? -gEncoderVal_right : gEncoderVal_right;
            gEncoderCount_right = 0;
            // printf("%d %d %d\r\n",gEncoderVal_left,gEncoderVal_right,xunji());
            if (timebegin==1)
            {
                if (timenum==2)
                {
                    whiteflag=1;
                    timebegin=0;
                    timenum=0;
                }
                timenum++;
            }

            // if (timebegin1==1)
            // {
            //     if (timenum1==1)
            //     {
            //         whiteflag1=1;
            //         timebegin1=0;
            //         timenum1=0;
            //     }
            //     timenum1++;
            // }

            if (ledbegin==1)
            {
                ledflag=1;
                if (lednum==10)
                {
                    ledflag=0;
                    ledbegin=0;
                    lednum=0;
                }
                lednum++;
            }
            break;
        default:
            break;
    }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST))
    {
        case DL_TIMER_IIDX_ZERO:
            g_sys_tick_10ms++;                  // 10ms 滴答，用于 MPU6050 实际 DT

            if (timebegin1==1)
            {
                if (timenum1==3)
                {
                    whiteflag1=1;
                    timebegin1=0;
                    timenum1=0;
                }
                timenum1++;
            }

            if (timebegin2==1)
            {
                if (timenum2==19)
                {
                    whiteflag2=1;
                    timebegin2=0;
                    timenum2=0;
                }
                timenum2++;
            }

            if (timbegin==1)
            {
                tim = tim + 0.01;
            }
            
            break;
        default:
            break;
    }
}

float PWM_Limit(float IN,float max,float min)                   //pwm限幅
{
	float OUT = IN;
	if(OUT > max) OUT = max;
	if(OUT < min) OUT = min;
	return OUT;
}

int myabs(int a)                                                //自定义绝对值函数
{
	int temp;
	if(a < 0)  temp = -a;
	else temp = a;
	return temp;
}


