#include "NonVolatileMemory.h"
#include "DataTypes.h"
#include "RS485.h"
#include "Console.h"
#include "I2C.h"
#include "Eeprom.h"

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

#define TIME_IN_BOOT_SAFETY_FAIL 60ul
#define RECV 0
#define PROCESS_SEND 1
#define BLVERSION_MAJOR 2
#define BLVERSION_MINOR 4

unsigned32 tick_counter = 0ul;
unsigned32 bootloader_timeout = 0ul;
boolean processing = true;
boolean reset_bsc = false;
unsigned8 version[3];
unsigned8 status = 0;
boolean status_led = true;

void main(void) {
    unsigned8 safety_counter = read_eeprom_byte(EEPROM_ADDR_SAFETY_BYTE);
    unsigned8 mode = RECV;

    INTCONbits.GIE = 0;
    WDTCONbits.SWDTEN = 0;

    read_eeprom(module_address, EEPROM_ADDR_ADDR, 4); // Read address
    read_eeprom(version, EEPROM_ADDR_BFW_VERSION_MAJOR, 2); // Read bootloader version
    if (version[0] != BLVERSION_MAJOR || version[1] != BLVERSION_MINOR) {
        version[0] = BLVERSION_MAJOR;
        version[1] = BLVERSION_MINOR;
        write_eeprom(version, EEPROM_ADDR_BFW_VERSION_MAJOR, 2);
    }
    read_eeprom(version, EEPROM_ADDR_FW_VERSION_MAJOR, 3); // Read application version

    init_uart();
    init_debug_uart();
    printf("\r\n\r\n--\r\nBLO\r\n");
    
    init_i2c();
    write_orange_status_led(status_led);
    
    printf("BVE %u.%u\r\n", BLVERSION_MAJOR, BLVERSION_MINOR);

    processing = false;
    reset_bsc = false;
    if ((module_address[0] == 0 || module_address[0] == 255) && safety_counter > 0) {
        // Device not initialized: Calculate application CRC and jump to application
        printf("FLO INIT\r\n");
        if (safety_counter > 5) {
            // However, only try a few times
            reset_bsc = true;
        }
        calculate_and_save_crc();
    } else if (safety_counter == 0) {
        // Safety counter at 0, application did not start.
        printf("FLO SFAIL\r\n");
        processing = true;
        bootloader_timeout = TIME_IN_BOOT_SAFETY_FAIL;
    } else {
        // Everything as expected, using configured timeout
        printf("FLO NORM\r\n");
        bootloader_timeout = read_eeprom_byte(EEPROM_ADDR_TIME_IN_BOOT);
        if (bootloader_timeout > 0ul) {
            processing = true;
        }
    }
    
    printf("BLT %lu\r\n", bootloader_timeout);
    printf("BSC %u\r\n", safety_counter);
    printf("FWV %u.%u.%u\r\n", version[0], version[1], version[2]);
    printf("ADR %u.%u.%u.%u\r\n", module_address[0], module_address[1], module_address[2], module_address[3]);
  
    bootloader_timeout <<= 1; // Because the tick is every half second
    start_counter();

    printf("PSG\r\n");
    while (processing) {
        ClrWdt(); // Clear the watchdog timer
        
        if (INTCONbits.TMR0IF == 1) {
            status_led = !status_led;
            write_orange_status_led(status_led);
            reset_counter();
            if (tick_counter++ >= bootloader_timeout) {
                printf("BTO\r\n");
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
                printf(" %u\r\n", error);
                
            default:
                mode = RECV;
                break;
        }
    }

    write_orange_status_led(false);
    printf("STP\r\n");

    if (!valid_code()) {
        printf("IVC\r\n");
        Reset();
    }

    if (reset_bsc) {
        printf("RSC\r\n");
        safety_counter = 5;
        write_eeprom(&safety_counter, EEPROM_ADDR_SAFETY_BYTE, 1);
    } else if (safety_counter > 0) {
        printf("DSC\r\n");
        safety_counter -= 1;
        write_eeprom(&safety_counter, EEPROM_ADDR_SAFETY_BYTE, 1);
    }
    
    printf("GTA\r\n\r\n\r\n");

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
    
    printf(" %lu", addr.value);

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
        printf(" CRE\r\n");
        return true;
    }
    
    return false;
}

void process_data() {
    error = NO_ERROR;
    if (received_command_first != 'F') {
        error = ERROR_CMD_NOT_RECOGNIZED;
    }

    printf("RCV %c%c", received_command_first, received_command_second);

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
            reset_bsc = true;
            processing = false;
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
