#include "BLDCDriver6PWM.h"

#define PWM_TIM htim1 // PWM定时器接口(本项目使用htim1，大家根据自己的项目外设接口修改)

BLDCDriver6PWM::BLDCDriver6PWM(){
    // Pin initialization
    // default power-supply value
    voltage_power_supply = DEF_POWER_SUPPLY;
    voltage_limit = NOT_SET;
    pwm_frequency = NOT_SET;
    // dead zone initial - 2%
    dead_zone = 0.02f;
}

// enable motor driver
void  BLDCDriver6PWM::enable(){
    setPwm(0, 0, 0);
    HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_1); // 启动定时器通道1
    HAL_TIMEx_PWMN_Start(&PWM_TIM, TIM_CHANNEL_1); // 启动定时器通道1

    HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_2); // 启动定时器通道2
    HAL_TIMEx_PWMN_Start(&PWM_TIM, TIM_CHANNEL_2); // 启动定时器通道2

    HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_3); // 启动定时器通道3
    HAL_TIMEx_PWMN_Start(&PWM_TIM, TIM_CHANNEL_3); // 启动定时器通道3
}

// disable motor driver
void BLDCDriver6PWM::disable()
{
    setPwm(0, 0, 0);
    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_1); // 停止定时器通道1
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_1); // 停止定时器通道1

    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_2); // 停止定时器通道2
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_2); // 停止定时器通道2

    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_3); // 停止定时器通道3
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_3); // 停止定时器通道3
}

// init hardware pins
int BLDCDriver6PWM::init() {
    // sanity check for the voltage limit configuration
    if( !_isset(voltage_limit) || voltage_limit > voltage_power_supply) voltage_limit =  voltage_power_supply;

    pwm_frequency = __HAL_TIM_GET_AUTORELOAD(&PWM_TIM); //! 获取定时器自动重装载寄存器值
    voltage_limit = DEF_POWER_SUPPLY; //! 设置电压限制为默认值

    HAL_TIM_Base_Start(&PWM_TIM); // 启动定时器

    //! 停止定时器通道1、2、3
    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_1); // 停止定时器通道1
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_1); // 停止定时器通道1

    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_2); // 停止定时器通道2
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_2); // 停止定时器通道2

    HAL_TIM_PWM_Stop(&PWM_TIM, TIM_CHANNEL_3); // 停止定时器通道3
    HAL_TIMEx_PWMN_Stop(&PWM_TIM, TIM_CHANNEL_3); // 停止定时器通道3

    // set zero to PWM
    dc_a = dc_b = dc_c = 0;

    //! 初始化成功
    SEGGER_RTT_printf(0, "BLDCDriver6PWM init success, frequency: %d, voltage_limit: ", pwm_frequency);
    SEGGER_Printf_Float(voltage_limit);
    initialized = 1;
    return 1;
}

// Set voltage to the pwm pin
void BLDCDriver6PWM::setPwm(float Ua, float Ub, float Uc) {
    // limit the voltage in driver
    Ua = _constrain(Ua, 0, voltage_limit);
    Ub = _constrain(Ub, 0, voltage_limit);
    Uc = _constrain(Uc, 0, voltage_limit);
    // calculate duty cycle
    // limited in [0,1]
    dc_a = _constrain(Ua / voltage_power_supply, 0.0f , 1.0f );
    dc_b = _constrain(Ub / voltage_power_supply, 0.0f , 1.0f );
    dc_c = _constrain(Uc / voltage_power_supply, 0.0f , 1.0f );
    // hardware specific writing
    // hardware specific function - depending on driver and mcu
    _writeDutyCycle3PWM_6PWM(&PWM_TIM, dc_a, dc_b, dc_c); // 自己编写的HAL库驱动，目的是设置三相PWM的占空比
}

// Set the phase state
// actually changing the state is only done on the next call to setPwm, and depends
// on the hardware capabilities of the driver and MCU.
void BLDCDriver6PWM::setPhaseState(PhaseState sa, PhaseState sb, PhaseState sc) {
//  phase_state[0] = sa;
//  phase_state[1] = sb;
//  phase_state[2] = sc;
}
