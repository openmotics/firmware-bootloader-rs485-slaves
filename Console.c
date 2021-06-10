#include "Console.h"
#include "RS485.h"
#include "NonVolatileMemory.h"

char console_stream[CONSOLE_STREAM_SIZE];
char* cs_begin = &console_stream[0];
char* cs_end = &console_stream[CONSOLE_STREAM_SIZE-1];
char* cs_read = &console_stream[0];
char* cs_write = &console_stream[0];

void init_debug_uart() {
    unsigned32_mask baudrate;
    baudrate.value = (SYSTEM_CLOCK/4)/CONS_BAUDRATE-1;
    
    BAUDCON2bits.BRG16 = 1;
    TXSTA2bits.BRGH = 1;
    SPBRG2 = baudrate.bytes[0];
    SPBRGH2 = baudrate.bytes[1];    
    TXSTA2bits.TXEN = 1;
    TXSTA2bits.SYNC = 0;
    RCSTA2bits.SPEN = 1;
    RCSTA2bits.CREN = 1;
}

void putch(char c)
{
    while(CONS_TRMTDONE == 0);
    CONS_TXREG = c;
    while(CONS_TRMTDONE == 0);
}
