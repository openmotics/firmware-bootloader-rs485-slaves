#include "NonVolatileMemory.h"
#include "DataTypes.h"
#include "RS485.h"
#include "Console.h"

#pragma config FOSC = HS2    // Clock in external mode with range including 10MHz   [HS2]INTIO2
#pragma config SOSCSEL = DIG
#pragma config PLLCFG = ON   // x4 PLL clock multiplication [ON]OFF
#pragma config XINST = OFF
#pragma config WDTEN = ON    // Enable watchdog timer SWDTEN        
#pragma config WDTPS = 512   // Set the watchdog timer prescaler to 4ms * 1024 = about 4 seconds
#pragma config CPB = OFF     // Boot memory read protect
#pragma config CP0 = OFF     // Program memory read protect
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config CPD = OFF     // EEPROM read protect

#define EEPROM_ADDR_ADDR 1                // 4 bytes with the address used in the program itself
#define EEPROM_ADDR_HW_VERSION 64         // Used to print this info when starting up.
#define EEPROM_ADDR_FW_VERSION_MAJOR 65
#define EEPROM_ADDR_FW_VERSION_MINOR 66
#define EEPROM_ADDR_FW_VERSION_BUILD 67
#define EEPROM_ADDR_CRC 68                // This 4 bytes are the checksum of the program code. should be match
#define EEPROM_ADDR_START_ADDR 72         // Not in use
#define EEPROM_ADDR_FLASHMODE 74          // Challenge for app to reset
#define EEPROM_ADDR_TIME_IN_BOOT 75       // Time to wait till timeout.
#define EEPROM_ADDR_STATUS 76             // Status of code with crc

#define START_CODE 0x00000000ul
#define END_CODE (1ul * PROG_NUM_OF_BLOCKS * BLOCK_SIZE)
#define START_BOOTLOADER (END_CODE)

// Interrupt-related
#define AppHighIntVector (0x4008)
#pragma code high_vector = 0x08
void interrupt_at_high_vector(void) {
    asm("GOTO AppHighIntVector");
}
#pragma code

#define AppLowIntVector (0x4018)
#pragma code low_vector = 0x18
void interrupt_at_low_vector(void) {
    asm("GOTO AppLowIntVector");
}
#pragma code

//----------------------------------------------------------------------------------------------------------------------

unsigned8 valid_code(void);
unsigned32 calculate_check(unsigned32 program_start, unsigned32 program_stop);
void read_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 number);
unsigned8 read_eeprom_byte(unsigned16 address);
void write_eeprom(unsigned8 *data, unsigned16 source_address, unsigned16 size); 
void unlock_and_activate(void);
void start_counter(void);
void reset_counter(void);
void process_data(void);
void calculate_and_save_crc(void);

enum {
    NO_ERROR = 0,
    ERROR_CMD_NOT_RECOGNIZED = 1,
    ERROR_OUT_OF_BOUNCE = 2,
    ERROR_FORMAT_NOK = 3,
    ERROR_CRC_NOK = 4,
    ERROR_PROG_CRC_NOK = 5,
    SEND_ADDR = 6
} ERROR_MESSAGES;

#define MODE_APP_CHALLENGE 4
#define TIME_IN_BL_CHALLENGE_FAIL 10ul
#define RECV 0
#define PROCESS_SEND 1
#define BLVERSION_MAJOR 2
#define BLVERSION_MINOR 2

unsigned32 tick_counter = 0ul;
unsigned32 bootloader_timeout = 0ul;
unsigned8 processing = true;
unsigned8 version[3];
unsigned8 status = 0;

void main(void)
{
    unsigned8 flash_mode = read_eeprom_byte(EEPROM_ADDR_FLASHMODE);
    unsigned8 mode = RECV;

    INTCONbits.GIE = 0;
    WDTCONbits.SWDTEN = 0;

    read_eeprom(module_address, EEPROM_ADDR_ADDR, 4); // Read address
    read_eeprom(version, EEPROM_ADDR_FW_VERSION_MAJOR, 3); // Read version

    init_uart();
    init_debug_uart();
    
    debug_print_str("\n\n-- BL\n", 1);

    processing = false;

    if ((module_address[0] == 0 || module_address[0] == 255) && flash_mode != MODE_APP_CHALLENGE) {
        // If the device is not initialized, calculate CRC and go to APP mode,
        // but prevent bootloader loops
        debug_print_str("FL A\n", 1);
        calculate_and_save_crc();
        processing = false; // Don't wait in BL
    } else if (flash_mode == MODE_APP_CHALLENGE) {
        debug_print_str("FL B\n", 1);
        processing = true;
        bootloader_timeout = TIME_IN_BL_CHALLENGE_FAIL;
    } else {
        debug_print_str("FL C\n", 1);
        bootloader_timeout = read_eeprom_byte(EEPROM_ADDR_TIME_IN_BOOT);
        if (bootloader_timeout > 0ul) {
            processing = true;
        }
    }
    
    debug_print_str("BT ", 1);
    debug_print_long(bootloader_timeout, 1);
    debug_print_str("\nBV ", 1);
    debug_print_byte(BLVERSION_MAJOR, 1);
    debug_print_str(".", 1);
    debug_print_byte(BLVERSION_MINOR, 1);
    debug_print_str("\nFV ", 1);
    for (unsigned8 i = 0; i < 3; i++) {
        debug_print_byte(version[i], 1);
        if (i < 2) {
            debug_print_str(".", 1);
        }
    }
    debug_print_str("\nAD ", 1);
    for (unsigned8 i = 0; i < 4; i++) {
        debug_print_byte(module_address[i], 1);
        if (i < 3) {
            debug_print_str(".", 1);
        }
    }
  
    bootloader_timeout <<= 1; // Because the tick is every half second
    start_counter();

    debug_print_str("\nPG\n", 1);
    while(processing) {
        ClrWdt(); // Clear the watchdog timer
        
        if (INTCONbits.TMR0IF == 1) {
            // TODO: Add "in bootloader"-indication
            reset_counter();
            if (tick_counter++ >= bootloader_timeout) {
                debug_print_str("TT\n", 1);
                processing = false;
            }
        }
        
        switch(mode){
            case RECV:
                if (receive_data()) {
                    mode = PROCESS_SEND;
                }
                break;
            case PROCESS_SEND:
                process_data();
                                
                if (error != ERROR_CMD_NOT_RECOGNIZED) {
                    tick_counter = 0;
                }

                send_data();
                debug_print_str(" ", 1);
                debug_print_byte(error, 1);
                debug_print_str("\n", 1);
                
            default:
                mode = RECV;
                break;
        }
    }

    if (flash_mode != MODE_APP_CHALLENGE) {
        flash_mode = MODE_APP_CHALLENGE;
        write_eeprom(&flash_mode, EEPROM_ADDR_FLASHMODE, 1);
    }

    debug_print_str("SP\n", 1);

    if (!valid_code()) {
        Reset();
    }
    debug_print_str("GA\n\n\n", 1);

    ClrWdt();
    WDTCONbits.SWDTEN = 1;
    ClrWdt();

    asm("goto 0xE678");
}

unsigned8 save_block(void) {
    ureg32 addr;
 
    unsigned8 buff[8];
    unsigned8 i = 0;

    unsigned16 page_to_erase;

    addr.value = 0ul;
    addr.bytes[1] = received_data[0];
    addr.bytes[0] = received_data[1];  
    
    debug_print_str(" ", 1);
    debug_print_long(addr.value, 1);

    page_to_erase = addr.LW;

    if (addr.value >= PROG_NUM_OF_BLOCKS) {
        return ERROR_OUT_OF_BOUNCE;
    }

    addr.value = addr.value * BLOCK_SIZE;
    if (addr.value == 0ul) {
        // Read first 8 bytes and save it in the buffer.
        read_program_memory((unsigned8*)buff, addr, 8u);
        for(i = 0; i < 8; i++) {
            received_data[i+2] = buff[i];
        }
    }

    erase_program_memory(page_to_erase);    
    write_program_memory(received_data + 2, addr);
    send_data_counter=0;
    return NO_ERROR;
}

unsigned8 save_version(void) {
    write_eeprom(received_data, EEPROM_ADDR_FW_VERSION_MAJOR, 3); 
    send_data_counter = 0;
    return NO_ERROR;
}

unsigned8 save_crc(void) {
    write_eeprom(received_data, EEPROM_ADDR_CRC, 4); 
    send_data_counter = 0;
    return NO_ERROR;
}

unsigned8 update_status(void) {
    unsigned8* ptr_status = &status;
    if (valid_code()) {
        status = 0;
        write_eeprom(ptr_status, EEPROM_ADDR_STATUS, 1); 
        return NO_ERROR;
    } else {
        status = 1;
        write_eeprom(ptr_status, EEPROM_ADDR_STATUS, 1); 
        return ERROR_PROG_CRC_NOK;
    }
}

unsigned8 get_firmware_version_and_status(void) {    
    send_data_counter=0;
    raw_send_data[send_data_counter++] = read_eeprom_byte(EEPROM_ADDR_HW_VERSION);
    raw_send_data[send_data_counter++] = read_eeprom_byte(EEPROM_ADDR_FW_VERSION_MAJOR);
    raw_send_data[send_data_counter++] = read_eeprom_byte(EEPROM_ADDR_FW_VERSION_MINOR);
    raw_send_data[send_data_counter++] = read_eeprom_byte(EEPROM_ADDR_FW_VERSION_BUILD);
    raw_send_data[send_data_counter++] = read_eeprom_byte(EEPROM_ADDR_STATUS);
    return NO_ERROR;
}

unsigned8 get_bootloader_version(void) {
    send_data_counter = 0;
    raw_send_data[send_data_counter++] = BLVERSION_MAJOR;
    raw_send_data[send_data_counter++] = BLVERSION_MINOR;
    return NO_ERROR;
}

unsigned8 command_crc_check(unsigned8 place) {
    unsigned8 i = 0;
    unsigned16_mask crc = {.value = 0};
    if (received_data[place] != 'C') {
        error = ERROR_FORMAT_NOK;
    }
    
    for(i = 0; i < place; i++) {
        received_crc += received_data[i];
    }

    crc.bytes[1] = received_data[place+1];
    crc.bytes[0] = received_data[place+2];    

    if (crc.value != received_crc) {
        error = ERROR_CRC_NOK;
    }
    
    if (error != NO_ERROR) {
        debug_print_str(" CRE", 1);
        return true;
    }
    
    return false;
}

void process_data() {
    error = NO_ERROR;
    if (received_command_first != 'F') {
        error = ERROR_CMD_NOT_RECOGNIZED;
    }

    debug_print_str("RV ", 1);
    debug_print_chr(received_command_first, 1);
    debug_print_chr(received_command_second, 1);

    switch(received_command_second){
        case 'N':
            if (command_crc_check(3)) {
                break;
            }
            error = save_version();
            break;
        case 'C':
            if (command_crc_check(4)) {
                break;
            }
            error = save_crc();
            break;
        case 'H':
            if (command_crc_check(0)) {
                break;
            }
            error = get_bootloader_version();
            break;
        case 'D':    
            if (command_crc_check(66)) {
                break;
            }
            error = save_block();
            break;
        case 'E':
            if (command_crc_check(0)) {
                break;
            }
            error = update_status();
            break;
        case 'V':
            if (command_crc_check(0)) {
                break;
            }
            error = get_firmware_version_and_status();
            break;
        case 'G':
            if (command_crc_check(0)) {
                break;
            }
            processing = 0;
            break;        
        case 'R':
            // Already in bootloader
            break;
        default:    
            error = ERROR_CMD_NOT_RECOGNIZED;
            break;
    }
}

void start_counter() {
    T0CONbits.TMR0ON = 0;
    T0CONbits.T08BIT = 0; // 16bit
    T0CONbits.T0CS = 0;
    T0CONbits.T0SE = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 7;
    reset_counter();
}

void reset_counter() {
    T0CONbits.TMR0ON = 0;
    INTCONbits.TMR0IF = 0;
    TMR0H = 0xB3; // 40000000 (4MHz) / 4 / 256 (prescaler) / 2 (every half second) = 19531 -> 65535 - 19531 = 46004                    
    TMR0L = 0xB4;
    T0CONbits.TMR0ON = 1;
}

#define start 0x0000
#define offset 0x3C40
 
unsigned8 valid_code(void) {
    ureg32 CRC;
    ureg32 CRC2;

    read_eeprom(CRC2.bytes, EEPROM_ADDR_CRC, 4);

    // Convert MSB lSB
    CRC.bytes[0] = CRC2.bytes[3];
    CRC.bytes[1] = CRC2.bytes[2];
    CRC.bytes[2] = CRC2.bytes[1];
    CRC.bytes[3] = CRC2.bytes[0];
    
    return (CRC.value == calculate_check(START_CODE + 8, END_CODE));
}

void calculate_and_save_crc() {
    ureg32 CRC;
    ureg32 CRC2;
    CRC2.value = calculate_check(START_CODE + 8, END_CODE);

    // Convert MSB lSB
    CRC.bytes[0] = CRC2.bytes[3];
    CRC.bytes[1] = CRC2.bytes[2];
    CRC.bytes[2] = CRC2.bytes[1];
    CRC.bytes[3] = CRC2.bytes[0];

    write_eeprom(&(CRC.bytes[0]), EEPROM_ADDR_CRC, 4);
}

// ProgramMemStart should be a multiple of BLOCK_SIZE
unsigned32 calculate_check(unsigned32 program_start, unsigned32 program_stop) {
    ureg32 addr;
    unsigned8 i = program_start;
    unsigned32 sum = 0;

    addr.value = 0;

    read_program_memory((unsigned8*)received_data, addr, BLOCK_SIZE); // The received_data is not used here any more, so this mem can be used

    for (addr.value = program_start; addr.value < program_stop; addr.value++) {
        if ((addr.value % BLOCK_SIZE) == 0) {
            i = 0;
            read_program_memory((unsigned8*)received_data, addr, BLOCK_SIZE); // The received_data is not used here any more, so this mem can be used
        }
        sum += received_data[i++];
    }

    return sum;
}

#pragma code

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

void erase_program_memory(unsigned16 page) {
    ClrWdt();
    TBLPTR = ((unsigned24)page << 6);
    EECON1 = 0b10010100; // Prepare for erasing flash memory
    unlock_and_activate();
}

void write_program_memory(unsigned8 *data, ureg32 source_address) {
    unsigned16 i;
    TBLPTR = (unsigned24)source_address.value;
    for(i = 0; i < (unsigned16)BLOCK_SIZE; i++) { // Load the programming latches
        TABLAT = data[i];
        asm("tblwtpostinc");
    }

    asm("tblrdpostdec"); // Do this instead of TBLPTR--; since it takes less code space.

    EECON1 = 0b10100100; // Flash programming mode
    unlock_and_activate();
}

void read_program_memory(unsigned8 *data, ureg32 source_address, unsigned16 num) {
    unsigned16 i;

    if (source_address.bytes[2] != 0xF0) {
        TBLPTR = (unsigned24)source_address.value;
    }

    for(i = 0; i < num; i++) {
        asm("tblrdpostinc");

        // Since 0x300004 and 0x300007 are not implemented we need to return 0xFF
        // Since the device reads 0x00 but the hex file has 0x00
        if(TBLPTRU == 0x30) {
            if (TBLPTRL == 0x05) {
                TABLAT = 0xFF;
            }
            if (TBLPTRL == 0x08) {
                TABLAT = 0xFF;            
            }
        }
        data[i] = TABLAT;
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
