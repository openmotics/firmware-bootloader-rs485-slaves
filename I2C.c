#include "I2C.h"
#include "Console.h"
#include "Eeprom.h"

unsigned char module_type = 0;

void i2c_configure()
{
  SSPCON1 = 0b00101000;  // Set SSPEN and setting SSPM to I2C Master mode, expecting clock = FOSC/(4 * (SSPADD + 1))
  SSPCON2 = 0;
  SSPADD = (/* clock frequency */4000000/(4 * /* i2c frequency */ 100000))-1;
  SSPSTAT = 0;
  TRISCbits.TRISC3 = 1;  // Set SDA/SCL direction
  TRISCbits.TRISC4 = 1;
}

void i2c_wait()
{
  while ((SSPSTAT & 0x04 /* R/~W: transmit in progress */) || (SSPCON2 & 0x1F /* anything is busy */));
}

void i2c_start()
{
  i2c_wait();    
  SSPCON2bits.SEN = 1;             //Initiate start condition
}

void i2c_repeated_start()
{
  i2c_wait();
  SSPCON2bits.RSEN = 1;           //Initiate repeated start condition
}

void i2c_stop()
{
  i2c_wait();
  SSPCON2bits.PEN = 1;           //Initiate stop condition
}

void i2c_write(unsigned char d)
{
  i2c_wait();
  SSPBUF = d;         //Write data to SSPBUF
}

void init_i2c(void) {
    i2c_configure();
    
    write_register(CHIP_0, DIRECTION_REGISTER_0, 0);
    write_register(CHIP_0, DIRECTION_REGISTER_1, 0);
    write_register(CHIP_1, DIRECTION_REGISTER_0, 0);
    write_register(CHIP_1, DIRECTION_REGISTER_1, 0);
    
    write_register(CHIP_0, OUTPUT_REGISTER_0, 0b11111111);
    write_register(CHIP_0, OUTPUT_REGISTER_1, 0b11111111);
    write_register(CHIP_1, OUTPUT_REGISTER_0, 0b11111111);
    write_register(CHIP_1, OUTPUT_REGISTER_1, 0b11111111);
    
    unsigned8 address_byte = read_eeprom_byte(EEPROM_ADDR_ADDR);
    module_type = 0;  // Unknown
    if (address_byte == 79) { // 79 == ord('O')
        module_type = 1;  // OMH06 expected
    } else if (address_byte < 255) {
        module_type = 2;  // OMH03 expected
    }
    
    printf("I2C OK\r\n");
}

void write_register(unsigned8 address, unsigned8 reg, unsigned8 data) {
    i2c_start();
    i2c_write(address);
    i2c_write(reg);
    i2c_write(data);
    i2c_stop();
}

void write_orange_status_led(boolean on) {
    if (module_type == 1) {
        if (on) {
            write_register(CHIP_1, OUTPUT_REGISTER_1, 0b11001111);
            //                            GR leds
        } else {
            write_register(CHIP_1, OUTPUT_REGISTER_1, 0b11111111);
            //                            GR leds
        }
    } else if (module_type == 2) {
        if (on) {
            write_register(CHIP_1, OUTPUT_REGISTER_1, 0b11111001);
            //                               GR leds
        } else {
            write_register(CHIP_1, OUTPUT_REGISTER_1, 0b11111111);
            //                               GR leds
        }
    }
}
