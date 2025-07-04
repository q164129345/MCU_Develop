#include "AS5600_I2C.h"

/** Typical configuration for the 12bit AMS AS5600 magnetic sensor over I2C interface */
struct AS5600_I2CConfig_s AS5600_I2C_Config = {
  .chip_address   = 0x36, // AS5600的I2C地址
  .bit_resolution = 12,   // 分辨率
  .angle_register = 0x0C, // 角度寄存器
  .data_start_bit = 11    // 数据起始位
};

/** Typical configuration for the 12bit AMS AS5048 magnetic sensor over I2C interface */
// MagneticSensorI2CConfig_s AS5048_I2C = {
//   .chip_address = 0x40,  // highly configurable.  if A1 and A2 are held low, this is probable value
//   .bit_resolution = 14,
//   .angle_register = 0xFE,
//   .data_start_bit = 15
// };

// MagneticSensorI2C(uint8_t _chip_address, float _cpr, uint8_t _angle_register_msb)
//  @param _chip_address  I2C chip address
//  @param _bit_resolution  bit resolution of the sensor
//  @param _angle_register_msb  angle read register
//  @param _bits_used_msb number of used bits in msb
AS5600_I2C::AS5600_I2C(uint8_t _chip_address, int _bit_resolution, uint8_t _angle_register_msb, int _bits_used_msb){
    // chip I2C address
    chip_address = _chip_address;
    // angle read register of the magnetic sensor
    angle_register_msb = _angle_register_msb;
    // register maximum value (counts per revolution)
    cpr = _powtwo(_bit_resolution);

    // depending on the sensor architecture there are different combinations of
    // LSB and MSB register used bits
    // AS5600 uses 0..7 LSB and 8..11 MSB
    // AS5048 uses 0..5 LSB and 6..13 MSB
    // used bits in LSB
    lsb_used = _bit_resolution - _bits_used_msb;
    // extraction masks
    lsb_mask = (uint8_t)( (2 << lsb_used) - 1 );
    msb_mask = (uint8_t)( (2 << _bits_used_msb) - 1 );
    //wire = &Wire;
}

AS5600_I2C::AS5600_I2C(AS5600_I2CConfig_s config){
    chip_address = config.chip_address; 

    // angle read register of the magnetic sensor
    angle_register_msb = config.angle_register;
    // register maximum value (counts per revolution)
    cpr = _powtwo(config.bit_resolution);

    int bits_used_msb = config.data_start_bit - 7;
    lsb_used = config.bit_resolution - bits_used_msb;
    // extraction masks
    lsb_mask = (uint8_t)( (2 << lsb_used) - 1 );
    msb_mask = (uint8_t)( (2 << bits_used_msb) - 1 );
    //wire = &Wire;
}

// 初始化
void AS5600_I2C::init(I2C_HandleTypeDef* _hi2c1){
  hi2c1 = _hi2c1;
  this->Sensor::init(); // call base class init
}

//  Shaft angle calculation
//  angle is in radians [rad]
float AS5600_I2C::getSensorAngle(){
  // (number of full rotations)*2PI + current sensor angle 
  return  ( getRawCount() / (float)cpr) * _2PI ;
}

// function reading the raw counter of the magnetic sensor
int AS5600_I2C::getRawCount(){
    return (int)AS5600_I2C::read(angle_register_msb);
}

// I2C functions
/*
* Read a register from the sensor
* Takes the address of the register as a uint8_t
* Returns the value of the register
*/
int AS5600_I2C::read(uint8_t angle_reg_msb) {
    uint8_t buffer[2]; // 用于存储读取的数据
    HAL_StatusTypeDef status;
    // 发送寄存器地址
    status = HAL_I2C_Mem_Read(hi2c1, chip_address << 1, angle_reg_msb, I2C_MEMADD_SIZE_8BIT, buffer, 2, 50);
    if (status != HAL_OK) {
        // 处理错误
        return -1; // 返回错误代码
    }
    // 将读取的两个字节组合成一个整数
    int value = (buffer[0] << 8) | buffer[1];
    return value;
}

/*
* Checks whether other devices have locked the bus. Can clear SDA locks.
* This should be called before sensor.init() on devices that suffer i2c slaves locking sda
* e.g some stm32 boards with AS5600 chips
* Takes the sda_pin and scl_pin
* Returns 0 for OK, 1 for other master and 2 for unfixable sda locked LOW
*/
// int AS5600_I2C::checkBus(byte sda_pin, byte scl_pin) {

//   pinMode(scl_pin, INPUT_PULLUP);
//   pinMode(sda_pin, INPUT_PULLUP);
//   delay(250);

//   if (digitalRead(scl_pin) == LOW) {
//     // Someone else has claimed master!");
//     return 1;
//   }

//   if(digitalRead(sda_pin) == LOW) {
//     // slave is communicating and awaiting clocks, we are blocked
//     pinMode(scl_pin, OUTPUT);
//     for (byte i = 0; i < 16; i++) {
//       // toggle clock for 2 bytes of data
//       digitalWrite(scl_pin, LOW);
//       delayMicroseconds(20);
//       digitalWrite(scl_pin, HIGH);
//       delayMicroseconds(20);
//     }
//     pinMode(sda_pin, INPUT);
//     delayMicroseconds(20);
//     if (digitalRead(sda_pin) == LOW) {
//       // SDA still blocked
//       return 2;
//     }
//     _delay(1000);
//   }
//   // SDA is clear (HIGH)
//   pinMode(sda_pin, INPUT);
//   pinMode(scl_pin, INPUT);

//   return 0;
// }
