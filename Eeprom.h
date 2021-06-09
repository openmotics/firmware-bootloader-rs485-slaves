#ifndef __EEPROM_H
    #define __EEPROM_H

    #include "DataTypes.h"

    #define EEPROM_ADDR_ADDR 1                // 4 bytes with the address used in the program itself
    #define EEPROM_ADDR_HW_VERSION 64         // Used to print this info when starting up.
    #define EEPROM_ADDR_FW_VERSION_MAJOR 65
    #define EEPROM_ADDR_FW_VERSION_MINOR 66
    #define EEPROM_ADDR_FW_VERSION_BUILD 67
    #define EEPROM_ADDR_CRC 68                // This 4 bytes are the checksum of the program code. should be match
    #define EEPROM_ADDR_START_ADDR 72         // Not in use
    #define EEPROM_ADDR_SAFETY_BYTE 74        // Challenge for app to reset
    #define EEPROM_ADDR_TIME_IN_BOOT 75       // Time to wait till timeout.
    #define EEPROM_ADDR_STATUS 76             // Status of code with crc
    #define EEPROM_ADDR_BFW_VERSION_MAJOR 100
    #define EEPROM_ADDR_BFW_VERSION_MINOR 101

    void read_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 num);
    unsigned8 read_eeprom_byte(unsigned16 address);
    void write_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 size);
    void unlock_and_activate(void);
#endif
