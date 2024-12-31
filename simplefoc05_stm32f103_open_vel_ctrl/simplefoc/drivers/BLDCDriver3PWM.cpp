#include "BLDCDriver3PWM.h"

#define PWM_TIM htim2 // pwm定时器接口（本项目使用htim2）

BLDCDriver3PWM::BLDCDriver3PWM(int phA, int phB, int phC, int en1, int en2, int en3){
  // Pin initialization
  pwmA = phA;
  pwmB = phB;
  pwmC = phC;

  // enable_pin pin
  enableA_pin = en1;
  enableB_pin = en2;
  enableC_pin = en3;

  // default power-supply value
  voltage_power_supply = DEF_POWER_SUPPLY;
  voltage_limit = NOT_SET;
  pwm_frequency = NOT_SET;
}

// enable motor driver
void  BLDCDriver3PWM::enable(){
  // enable_pin the driver - if enable_pin pin available
  HAL_GPIO_WritePin(m1_enable_GPIO_Port, m1_enable_Pin, GPIO_PIN_SET); // 使能半桥驱动芯片
  // set zero to PWM
  setPwm(0,0,0);
  SEGGER_RTT_printf(0,"enable motor.\n");
}

// disable motor driver
void BLDCDriver3PWM::disable()
{
  // set zero to PWM
  setPwm(0, 0, 0);
  // disable the driver - if enable_pin pin available
  HAL_GPIO_WritePin(m1_enable_GPIO_Port, m1_enable_Pin, GPIO_PIN_RESET); // 不使能半桥驱动芯片
  SEGGER_RTT_printf(0,"disable motor.\n");
}

// init hardware pins
int BLDCDriver3PWM::init() {
  // sanity check for the voltage limit configuration
  if(!_isset(voltage_limit) || voltage_limit > voltage_power_supply) voltage_limit =  voltage_power_supply;

  // Set the pwm frequency to the pins
  pwm_frequency = __HAL_TIM_GET_AUTORELOAD(&PWM_TIM); // 获取pwm频率，即ARR寄存器
  voltage_limit = DEF_POWER_SUPPLY; // 获取电压限制
  HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_1); // 使能A相
  HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_2); // 使能B相
  HAL_TIM_PWM_Start(&PWM_TIM, TIM_CHANNEL_3); // 使能C相
  SEGGER_RTT_printf(0, "pwm_frequency:%u,voltage_limit:", pwm_frequency);
  SEGGER_Printf_Float(voltage_limit);
  SEGGER_RTT_printf(0,"\n");
  return 1;
}

// Set voltage to the pwm pin
void BLDCDriver3PWM::setPhaseState(PhaseState sa, PhaseState sb, PhaseState sc) {
  // disable if needed
  // if( _isset(enableA_pin) &&  _isset(enableB_pin)  && _isset(enableC_pin) ){
  //   digitalWrite(enableA_pin, sa == PhaseState::PHASE_ON ? enable_active_high:!enable_active_high);
  //   digitalWrite(enableB_pin, sb == PhaseState::PHASE_ON ? enable_active_high:!enable_active_high);
  //   digitalWrite(enableC_pin, sc == PhaseState::PHASE_ON ? enable_active_high:!enable_active_high);
  // }
}

// Set voltage to the pwm pin
void BLDCDriver3PWM::setPwm(float Ua, float Ub, float Uc) {

  // limit the voltage in driver
  Ua = _constrain(Ua, 0.0f, voltage_limit);
  Ub = _constrain(Ub, 0.0f, voltage_limit);
  Uc = _constrain(Uc, 0.0f, voltage_limit);
  // calculate duty cycle
  // limited in [0,1]
  dc_a = _constrain(Ua / voltage_power_supply, 0.0f, 1.0f);
  dc_b = _constrain(Ub / voltage_power_supply, 0.0f, 1.0f);
  dc_c = _constrain(Uc / voltage_power_supply, 0.0f, 1.0f);

  // hardware specific writing
  // hardware specific function - depending on driver and mcu
  //_writeDutyCycle3PWM(dc_a, dc_b, dc_c, params);
  _writeDutyCycle3PWM(&PWM_TIM, dc_a, dc_b, dc_c);
}
