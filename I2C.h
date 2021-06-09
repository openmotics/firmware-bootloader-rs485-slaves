#ifndef __I2C_H
    #define __I2C_H

    #include "DataTypes.h"
 
    #define CHIP_0 0b11101100
    #define CHIP_1 0b11101010
    
    void init_i2c(void);
    void write_register(unsigned8 address, unsigned8 reg, unsigned8 data);
    void write_orange_status_led(boolean on);
#endif
