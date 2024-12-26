#ifndef AS5600_I2C_H
#define AS5600_I2C_H

#include "main.h"
//#include <Wire.h>
#include "Sensor.h"
#include "foc_utils.h"
#include "time_utils.h"

struct AS5600_I2CConfig_s  {
  int chip_address;
  int bit_resolution;
  int angle_register;
  int data_start_bit; 
};
// some predefined structures
// extern AS5600_I2CConfig_s AS5600_I2C;

class AS5600_I2C: public Sensor{
 public:
    /**
     * AS5600_I2C class constructor
     * @param chip_address  I2C chip address
     * @param bits number of bits of the sensor resolution 
     * @param angle_register_msb  angle read register msb
     * @param _bits_used_msb number of used bits in msb
     */
    AS5600_I2C(uint8_t _chip_address, int _bit_resolution, uint8_t _angle_register_msb, int _msb_bits_used);

    /**
     * AS5600_I2C class constructor
     * @param config  I2C config
     */
    AS5600_I2C(AS5600_I2CConfig_s config);

    // static AS5600_I2C AS5600();
        
    /** sensor initialise pins */
    //void init(TwoWire* _wire = &Wire); // 根据STM32的框架，重新写
    void init(I2C_HandleTypeDef* hi2c1);

    // implementation of abstract functions of the Sensor class
    /** get current angle (rad) */
    float getSensorAngle() override;

    /** experimental function to check and fix SDA locked LOW issues */
    //int checkBus(byte sda_pin , byte scl_pin );

    /** current error code from Wire endTransmission() call **/
    uint8_t currWireError = 0;

  private:
    float cpr; //!< Maximum range of the magnetic sensor
    uint16_t lsb_used; //!< Number of bits used in LSB register
    uint8_t lsb_mask;
    uint8_t msb_mask;
    
    // I2C variables
    uint8_t angle_register_msb; //!< I2C angle register to read
    uint8_t chip_address; //!< I2C chip select pins

    // I2C functions
    /** Read one I2C register value */
    int read(uint8_t angle_register_msb);

    /**
     * Function getting current angle register value
     * it uses angle_register variable
     */
    int getRawCount();
    
    /* the two wire instance for this sensor */
    //TwoWire* wire;
    I2C_HandleTypeDef* hi2c1;

};


#endif
