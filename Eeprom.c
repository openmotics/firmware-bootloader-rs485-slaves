#include "Eeprom.h"

void read_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 num) {
    unsigned16 i;
    
    for(i = 0; i < num; i++) {
        EEADR = (unsigned8)source_address;
        EEADRH = (unsigned8)(source_address >> 8);
        EECON1 = 0b00000000; // EEPROM read mode
        EECON1bits.RD = 1;
        Nop();
        data[i] = EEDATA;                    
        
        source_address++;
    }
}

unsigned8 read_eeprom_byte(unsigned16 address) {
    unsigned8 result;
    read_eeprom(&result, address, 1);
    return result;
}

void write_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 size) {
    unsigned16 i;
    for(i = 0; i < size; i++) {
        EEADR = (unsigned8)source_address;
        EEADRH = (unsigned8)(source_address >> 8);
        EEDATA = data[i];

        EECON1 = 0b00000100;    // EEPROM Write mode
        unlock_and_activate();
        source_address++;
    }
}

void unlock_and_activate(void) {
    boolean interrupts_enabled = INTCONbits.GIE;
    INTCONbits.GIE = 0; // Make certain interrupts disabled for unlock process.
    
    // Now unlock sequence to set WR (make sure interrupts are disabled before executing this)
    asm("MOVLW 0x55");
    asm("MOVWF EECON2");
    asm("MOVLW 0xAA");
    asm("MOVWF EECON2");
    asm("BSF EECON1, 1"); // Performs write

    if (interrupts_enabled) {
        INTCONbits.GIE = 1;
    }
    
    while(EECON1bits.WR); // Wait until complete (relevant when programming EEPROM, not important when programming flash since processor stalls during flash program)    
    EECON1bits.WREN = 0; // Good practice now to clear the WREN bit, as further protection against any accidental activation of self write/erase operations.
}    