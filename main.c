#include "NonVolatileMemory.h"
#include "DataTypes.h"
#include "RS485.h"
#include "Console.h"

#if (defined(OUTPUT_MODULE) || defined(INPUT_MODULE) || defined(DIMMER_MODULE))
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
#endif
#if (defined(CAN_CONTROL_MODULE))
    #pragma config FOSC = HS2    // Clock in external mode with range including 10MHz   [HS2]INTIO2
    #pragma config SOSCSEL = DIG
    #pragma config PLLCFG = ON   // x4 PLL clock multiplication [ON]OFF
    #pragma config XINST = OFF
    #pragma config WDTEN = ON    // Enable watchdog timer SWDTEN        
    #pragma config WDTPS = 512   // Set the watchdog timer prescaler to 4ms * 1024 = about 4 seconds
    #pragma config CANMX = PORTB // Mux cna on portb 
    #pragma config CPB = OFF     // Boot memory read protect
    #pragma config CP0 = OFF     // Program memory read protect
    #pragma config CP1 = OFF
    #pragma config CP2 = OFF
    #pragma config CP3 = OFF
    #pragma config CPD = OFF     // EEPROM read protect
#endif

#if (defined(OUTPUT_MODULE) || defined(INPUT_MODULE) || defined(DIMMER_MODULE))
    #define EEPROM_ADDR_ADDR 1                // 4 bytes with the address used in the program itself
    #define EEPROM_ADDR_HW_version 64         // Used to print this info when starting up.
    #define EEPROM_ADDR_FW_version_major 65
    #define EEPROM_ADDR_FW_version_minor 66
    #define EEPROM_ADDR_FW_version_builtnr 67
    #define EEPROM_ADDR_CRC 68                // This 4 bytes are the checksum of the program code. should be match
    #define EEPROM_ADDR_START_ADDR 72         // Not in use
    #define EEPROM_ADDR_FLASHMODE 74          // Challenge for app to reset
    #define EEPROM_ADDR_TIME_IN_BOOT 75       // Time to wait till timeout.
    #define EEPROM_ADDR_STATUS 76             // Status of code with crc
#endif
#if (defined(CAN_CONTROL_MODULE))
    #define EEPROM_ADDR_ADDR 309u               // 4 bytes with the address used in the program itself
    #define EEPROM_ADDR_HW_version 320u         // Used to print this info when starting up.
    #define EEPROM_ADDR_FW_version_major 321u
    #define EEPROM_ADDR_FW_version_minor 322u
    #define EEPROM_ADDR_FW_version_builtnr 323u
    #define EEPROM_ADDR_CRC 324u                // This 4 bytes are the checksum of the program code. should be match
    #define EEPROM_ADDR_START_ADDR 328u         // Not in use
    #define EEPROM_ADDR_FLASHMODE 329u          // Challenge for app to reset
    #define EEPROM_ADDR_TIME_IN_BOOT 330u       // Time to wait till timeout.
    #define EEPROM_ADDR_STATUS 331u             // Status of code with crc
#endif

#define StartCode 0x00000000ul
#define EndCode (1ul*PROG_NUM_OF_BLOCKS*BLOCK_SIZE)
#define StartBL (EndCode)

// Interrupt-related
#define AppHighIntVector (0x4008)

// void (*fpJumpToAppHighInt)(void) = AppHighIntVector;
#pragma code high_vector = 0x08
void interrupt_at_high_vector(void) {
    // asm("CALL AppHighIntVector,0");
    asm("GOTO AppHighIntVector");
    // (*fpJumpToAppHighInt)();
}
#pragma code

#define AppLowIntVector (0x4018)

// void (*fpJumpToAppLowInt)(void) = AppLowIntVector;
#pragma code low_vector = 0x18
void interrupt_at_low_vector(void) {
    // asm("CALL AppLowIntVector,0");
    asm("GOTO AppLowIntVector");
    // (*fpJumpToAppLowInt)();
}
#pragma code

//----------------------------------------------------------------------------------------------------------------------

#define MODE_APP_CHALLENGE 4

unsigned8 ValidCode(void);
unsigned32 CalcCheck(unsigned32 ProgramStart,unsigned32 ProgramStop);
void ReadEEPROM(unsigned8 * ptrData, unsigned16 SourceAddr, unsigned16 num);
unsigned8 ReadEEPROMByte(unsigned16 addr);
void WriteEEPROM(unsigned8 *ptrData, unsigned16 SourceAddr, unsigned16 size); 
void UnlockAndActivate(void);
unsigned8 CheckButton(void);
void StartTickCounter(void);
void ResetCounter(void);
void ProcessData(void);
void CalculateAndSaveCRC(void);

unsigned32 tickcounter = 0ul;
unsigned32 WDTTick = 0ul;

#define TIME_IN_BL_CHALLENGE_FAIL 10ul
#define TIME_IN_BL_IF_BUTTON_PRESSED 30ul

#if defined(OUTPUT_MODULE)    
    #define BUTTON_TRIS TRISAbits.TRISA5
    #define BUTTON PORTAbits.RA5
    #define LED_PWR_TRIS TRISEbits.TRISE2
    #define LED_PWR PORTEbits.RE2
    #define LED_STAT_TRIS TRISEbits.TRISE1
    #define LED_STAT PORTEbits.RE1
#endif
#if defined(INPUT_MODULE)    
    #define BUTTON_TRIS TRISBbits.TRISB1
    #define BUTTON PORTBbits.RB1
    #define LED_PWR_TRIS TRISEbits.TRISE2
    #define LED_PWR PORTEbits.RE2
    #define LED_STAT_TRIS TRISEbits.TRISE1
    #define LED_STAT PORTEbits.RE1
#endif
#if defined(DIMMER_MODULE)    
    #define BUTTON_TRIS TRISBbits.TRISB1
    #define BUTTON PORTBbits.RB1
    #define LED_PWR_TRIS TRISEbits.TRISE2
    #define LED_PWR PORTEbits.RE2
    #define LED_STAT_TRIS TRISEbits.TRISE1
    #define LED_STAT PORTEbits.RE1
#endif
#if defined(CAN_CONTROL_MODULE)    
    #define BUTTON_TRIS TRISBbits.TRISB1
    #define BUTTON PORTBbits.RB1             
    #define LED_PWR_TRIS TRISEbits.TRISE2
    #define LED_PWR PORTEbits.RE2
    #define LED_STAT_TRIS TRISEbits.TRISE1
    #define LED_STAT PORTEbits.RE1
#endif

enum {
    NO_ERROR = 0,
    ERROR_CMD_NOT_RECOGNIZED = 1,
    ERROR_OUT_OF_BOUNCE = 2,
    ERROR_FORMAT_NOK = 3,
    ERROR_CRC_NOK = 4,
    ERROR_PROG_CRC_NOK = 5,
    SEND_ADDR = 6
} ERROR_MESSAGES;

unsigned8 Processing = true;

#define RECV 0
#define PROCESS_SEND 1

unsigned8 version[3];

#define BLVERSION_MAJOR 2
#define BLVERSION_MINOR 0

unsigned8 status=0;

void main(void)
{
    unsigned8 StayInBoot = false;
    unsigned8 FlashMode = ReadEEPROMByte(EEPROM_ADDR_FLASHMODE);
    unsigned8 ButtonCounter = 0;
    unsigned8 mode = RECV;
    unsigned8 i;
    unsigned8 verb;
    uReg32 addr;

    INTCONbits.GIE = 0;
    WDTCONbits.SWDTEN = 0;

    LED_PWR_TRIS = 0;
    LED_STAT_TRIS = 0;

    ReadEEPROM(ADDR, EEPROM_ADDR_ADDR, 4); // Read address
    ReadEEPROM(version, EEPROM_ADDR_FW_version_major, 3); // Read version

    init_uart();
    InitDebugUART();
    
    DBGPrintSTR("\n\n-- BL ", 1);
    #if defined(OUTPUT_MODULE)    
        DBGPrintSTR("RY", 1);
    #endif
    #if defined(INPUT_MODULE)    
        DBGPrintSTR("IT", 1);
    #endif
    #if defined(DIMMER_MODULE)    
        DBGPrintSTR("DL", 1);
    #endif
    #if defined(CAN_CONTROL_MODULE)    
        DBGPrintSTR("CL", 1);
    #endif
    DBGPrintSTR("\n", 1);

    Processing = false;

    if (ADDR[0] == 0 || ADDR[0] == 255) { // If no address is assigned, the device is not yet initialized, go to app but first calculate the crc.
        DBGPrintSTR("FL A\n", 1);
        CalculateAndSaveCRC();
        Processing = false; // Dont wait in BL
    } else if (FlashMode == MODE_APP_CHALLENGE) {
        DBGPrintSTR("FL B\n", 1);
        Processing = true;
        WDTTick = TIME_IN_BL_CHALLENGE_FAIL;
    } else {
        DBGPrintSTR("FL C\n", 1);
        WDTTick = ReadEEPROMByte(EEPROM_ADDR_TIME_IN_BOOT);
        if (WDTTick > 0ul) {
            Processing = true;
        }
    }
    
    DBGPrintSTR("WT ", 1);
    DBGPrintLONG(WDTTick, 1);
    DBGPrintSTR("\nBV ", 1);
    DBGPrintBYTE(BLVERSION_MAJOR, 1);
    DBGPrintSTR(" ", 1);
    DBGPrintBYTE(BLVERSION_MINOR, 1);
    DBGPrintSTR("\nAR ", 1);
    for (unsigned8 i = 0; i < 4; i++) {
        DBGPrintBYTE(ADDR[i], 1);
        DBGPrintSTR(" ", 1);
    }
  
    WDTTick <<= 1; // Because the tick is every half second
    StartTickCounter();

    DBGPrintSTR("\nPG\n", 1);
    while(Processing){
        if (INTCONbits.TMR0IF == 1) {
            LED_PWR ^= 1;
            LED_STAT ^= 1;
            ResetCounter();
            if (tickcounter++ >= WDTTick) {
                DBGPrintSTR("TT\n", 1);
                Processing = false;
            }
        }
        
        switch(mode){
            case RECV:
                if (RecvData()) {
                    mode = PROCESS_SEND;
                }
                break;
            case PROCESS_SEND:
                ProcessData();
                                
                if (error != ERROR_CMD_NOT_RECOGNIZED) {
                    tickcounter=0;
                }

                SendData();
                DBGPrintSTR(" ", 1);
                DBGPrintBYTE(error, 1);
                DBGPrintSTR("\n", 1);
                
            default:
                mode=RECV;
                break;
        }
    }

    FlashMode = MODE_APP_CHALLENGE;
    WriteEEPROM(&FlashMode, EEPROM_ADDR_FLASHMODE, 1);

    DBGPrintSTR("SP\n", 1);

    if (!ValidCode()) {
        Reset();
    }
    DBGPrintSTR("GA\n\n\n", 1);

    ClrWdt();
    WDTCONbits.SWDTEN = 1;
    ClrWdt();

    #if defined(OUTPUT_MODULE)
        asm("goto 0x6678");
    #endif
    #if defined(INPUT_MODULE)
        asm("goto 0x6678");
    #endif
    #if defined(DIMMER_MODULE)
        asm("goto 0x6678");
    #endif
    #if defined(CAN_CONTROL_MODULE)
        asm("goto 0xE678");
    #endif
}

unsigned8 SaveBlock(void) {
    uReg32 addr;
 
    unsigned8 buff[8];
    unsigned8 i = 0;

    unsigned16 page_to_erase;

    addr.Val32 = 0ul;
    addr.Val[1] = RECV_Data[0];
    addr.Val[0] = RECV_Data[1];  
    
    DBGPrintSTR(" ", 1);
    DBGPrintLONG(addr.Val32, 1);

    page_to_erase = addr.LW;

    if (addr.Val32 >= PROG_NUM_OF_BLOCKS) {
        return ERROR_OUT_OF_BOUNCE;
    }

    addr.Val32 = addr.Val32 * BLOCK_SIZE;
    if (addr.Val32 == 0ul) {
        // Read first 8 bytes and save it in the buffer.
        ReadPMn((unsigned8*)buff, addr, 8u);
        for(i = 0; i < 8; i++) {
            RECV_Data[i+2] = buff[i];
        }
    }

    Erase(page_to_erase);    
    WritePM(RECV_Data + 2, addr);
    SendDataCount=0;
    return NO_ERROR;
}

unsigned8 SaveVersion(void) {
    WriteEEPROM(RECV_Data, EEPROM_ADDR_FW_version_major, 3); 
    SendDataCount = 0;
    return NO_ERROR;
}

unsigned8 SaveCRC(void) {
    WriteEEPROM(RECV_Data, EEPROM_ADDR_CRC, 4); 
    SendDataCount = 0;
    return NO_ERROR;
}

unsigned8 UpdateStatus(void) {
    unsigned8* pStatus = &status;
    if (ValidCode()) {
        status = 0;
        WriteEEPROM(pStatus, EEPROM_ADDR_STATUS, 1); 
        return NO_ERROR;
    } else {
        status = 1;
        WriteEEPROM(pStatus, EEPROM_ADDR_STATUS, 1); 
        return ERROR_PROG_CRC_NOK;
    }
}

unsigned8 GetFWVersionAndStatus(void) {    
    SendDataCount=0;
    SendDataRaw[SendDataCount++] = ReadEEPROMByte(EEPROM_ADDR_HW_version);
    SendDataRaw[SendDataCount++] = ReadEEPROMByte(EEPROM_ADDR_FW_version_major);
    SendDataRaw[SendDataCount++] = ReadEEPROMByte(EEPROM_ADDR_FW_version_minor);
    SendDataRaw[SendDataCount++] = ReadEEPROMByte(EEPROM_ADDR_FW_version_builtnr);
    SendDataRaw[SendDataCount++] = ReadEEPROMByte(EEPROM_ADDR_STATUS);
    return NO_ERROR;
}

unsigned8 GetBLVersion(void) {
    SendDataCount = 0;
    SendDataRaw[SendDataCount++] = BLVERSION_MAJOR;
    SendDataRaw[SendDataCount++] = BLVERSION_MINOR;
    return NO_ERROR;
}

unsigned8 CommCRCCheck(unsigned8 place) {
    unsigned8 i = 0;
    unsigned16_M crc = {.Val = 0};
    if (RECV_Data[place] != 'C') {
        error = ERROR_FORMAT_NOK;
    }
    
    for(i = 0; i < place; i++) {
        Recv_crc += RECV_Data[i];
    }

    crc.v[1] = RECV_Data[place+1];
    crc.v[0] = RECV_Data[place+2];    

    if (crc.Val != Recv_crc) {
        error = ERROR_CRC_NOK;
    }
    
    if (error != NO_ERROR) {
        //DBGPrintSTR("CRC E\n", 1);
        return true;
    }
    
    return false;
}

void ProcessData() {
    unsigned8 i = 0;

    error = NO_ERROR;
    if (RECV_comm != 'F') {
        error = ERROR_CMD_NOT_RECOGNIZED;
    }

    DBGPrintSTR("RV ", 1);
    DBGPrintCHR(RECV_comm, 1);
    DBGPrintCHR(RECV_command, 1);

    switch(RECV_command){
        case 'N':
            if (CommCRCCheck(3)) {
                break;
            }
            error = SaveVersion();
            break;
        case 'C':
            if (CommCRCCheck(4)) {
                break;
            }
            error = SaveCRC();
            break;
        case 'H':
            if (CommCRCCheck(0)) {
                break;
            }
            error = GetBLVersion();
            break;
        case 'D':    
            if (CommCRCCheck(66)) {
                break;
            }
            error = SaveBlock();
            break;
        case 'E':
            if (CommCRCCheck(0)) {
                break;
            }
            error = UpdateStatus();
            break;
        case 'V':
            if (CommCRCCheck(0)) {
                break;
            }
            error = GetFWVersionAndStatus();
            break;
        case 'G':
            if (CommCRCCheck(0)) {
                break;
            }
            Processing = 0;
            break;        
        case 'R':
            // Already in bootloader
            break;
        default:    
            error = ERROR_CMD_NOT_RECOGNIZED;
            break;
    }
}

void StartTickCounter() {
    T0CONbits.TMR0ON = 0;
    T0CONbits.T08BIT = 0; // 16bit
    T0CONbits.T0CS = 0;
    T0CONbits.T0SE = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 7;
    ResetCounter();
}

void ResetCounter() {
    T0CONbits.TMR0ON = 0;
    INTCONbits.TMR0IF = 0;
    TMR0H = 0xB3; // 40000000 (4MHz) / 4 / 256 (prescaler) / 2 (every half second) = 19531 -> 65535 - 19531 = 46004                    
    TMR0L = 0xB4;
    T0CONbits.TMR0ON = 1;
}

#define start 0x0000
#define offset 0x3C40
 
unsigned8 ValidCode(void)
{
    uReg32 CRC;
    uReg32 CRC2;
    unsigned8 result = 0;

    ReadEEPROM(CRC2.Val,EEPROM_ADDR_CRC,4);

    // Convert MSB lSB
    CRC.Val[0] = CRC2.Val[3];
    CRC.Val[1] = CRC2.Val[2];
    CRC.Val[2] = CRC2.Val[1];
    CRC.Val[3] = CRC2.Val[0];
    
    return (CRC.Val32 == CalcCheck(StartCode+8,EndCode));
}

void CalculateAndSaveCRC() {
    uReg32 CRC;
    uReg32 CRC2;
    CRC2.Val32 = CalcCheck(StartCode+8, EndCode);

    // Convert MSB lSB
    CRC.Val[0] = CRC2.Val[3];
    CRC.Val[1] = CRC2.Val[2];
    CRC.Val[2] = CRC2.Val[1];
    CRC.Val[3] = CRC2.Val[0];

    WriteEEPROM(&(CRC.Val[0]), EEPROM_ADDR_CRC,4);
}

// ProgramMemStart should be a multiple of BLOCK_SIZE
unsigned32 CalcCheck(unsigned32 ProgramStart, unsigned32 ProgramStop) {
    uReg32 addr;
    uReg32 test;
    unsigned8 i = ProgramStart;
    unsigned32 sum = 0;

    addr.Val32 = 0;

    ReadPMn((unsigned8*)RECV_Data, addr, BLOCK_SIZE); // The RECV_Data is not used here any more, so this mem can be used

    for (addr.Val32 = ProgramStart; addr.Val32 < ProgramStop; addr.Val32++) {
        if ((addr.Val32 % BLOCK_SIZE) == 0) {
            i = 0;
            ReadPMn((unsigned8*)RECV_Data, addr, BLOCK_SIZE); // The RECV_Data is not used here any more, so this mem can be used
        }
        sum += RECV_Data[i++];
    }

    return sum;
}

#pragma code

void ReadEEPROM(unsigned8 * ptrData, unsigned16 SourceAddr, unsigned16 num) {
    unsigned16 i;
    
    for(i = 0; i < num; i++) {
        EEADR = (unsigned8)SourceAddr;
        EEADRH = (unsigned8)(SourceAddr >> 8);
        EECON1 = 0b00000000; // EEPROM read mode
        EECON1bits.RD = 1;
        Nop();
        ptrData[i] = EEDATA;                    
        
        SourceAddr++;
    }
}

unsigned8 ReadEEPROMByte(unsigned16 addr) {
    unsigned8 result;
    ReadEEPROM(&result, addr, 1);
    return result;
}

void WriteEEPROM(unsigned8 *ptrData, unsigned16 SourceAddr, unsigned16 size) {
    unsigned16 i;
    for(i = 0; i < size; i++) {
        EEADR = (unsigned8)SourceAddr;
        EEADRH = (unsigned8)(SourceAddr >> 8);
        EEDATA = ptrData[i];

        EECON1 = 0b00000100;    // EEPROM Write mode
        UnlockAndActivate();
        SourceAddr++;
    }
}

void Erase(unsigned16 Page) {
    ClrWdt();
    TBLPTR = ((unsigned24)Page << 6);
    EECON1 = 0b10010100; // Prepare for erasing flash memory
    UnlockAndActivate();
}

void WritePM(unsigned8 * ptrData, uReg32 SourceAddr)
{
    unsigned16 i;
    TBLPTR = (unsigned24)SourceAddr.Val32;
    for(i = 0; i < (unsigned16)BLOCK_SIZE; i++) { // Load the programming latches
        TABLAT = ptrData[i];
        asm("tblwtpostinc");
    }

    asm("tblrdpostdec"); // Do this instead of TBLPTR--; since it takes less code space.

    EECON1 = 0b10100100; // Flash programming mode
    UnlockAndActivate();
}

void ReadPMn(unsigned8 * ptrData, uReg32 SourceAddr, unsigned16 num)
{
    unsigned16 i;

    if (SourceAddr.Val[2] != 0xF0) {
        TBLPTR = (unsigned24)SourceAddr.Val32;
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
        ptrData[i] = TABLAT;
    }
}

void UnlockAndActivate(void) {
    boolean interruptsEnabled = INTCONbits.GIE;
    INTCONbits.GIE = 0; // Make certain interrupts disabled for unlock process.
    
    // Now unlock sequence to set WR (make sure interrupts are disabled before executing this)
    asm("MOVLW 0x55");
    asm("MOVWF EECON2");
    asm("MOVLW 0xAA");
    asm("MOVWF EECON2");
    asm("BSF EECON1, 1"); // Performs write

    if (interruptsEnabled) {
        INTCONbits.GIE = 1;
    }
    
    while(EECON1bits.WR); // Wait until complete (relevant when programming EEPROM, not important when programming flash since processor stalls during flash program)    
    EECON1bits.WREN = 0; // Good practice now to clear the WREN bit, as further protection against any accidental activation of self write/erase operations.
}    
