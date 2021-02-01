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

    #if (COMM_UART_SEL == 1)
        BAUDCON2bits.BRG16 = 1;
        TXSTA2bits.BRGH = 1;
        SPBRG2 = baudrate.bytes[0];
        SPBRGH2 = baudrate.bytes[1];    
        TXSTA2bits.TXEN = 1;
        TXSTA2bits.SYNC = 0;
        RCSTA2bits.SPEN = 1;
        RCSTA2bits.CREN = 1;
    #else
        BAUDCON1bits.BRG16 = 1;
        TXSTA1bits.BRGH = 1;
        SPBRG1 = baudrate.v[0];
        SPBRGH1 = baudrate.v[1];
        TXSTA1bits.TXEN = 1;
        TXSTA1bits.SYNC = 0;
        RCSTA1bits.SPEN = 1;
        RCSTA1bits.CREN = 1;
    #endif
}

void debug_print_byte(unsigned8 data,char blocking){
    char text[4];
    sprintf(text, "%d", data);
    debug_print_str(text, blocking);
}

void debug_print_long(unsigned32 data,char blocking) {
    char text[12];
    sprintf(text, "%d", data);
    debug_print_str(text, blocking);
}

void debug_print_chr(unsigned8 data, char blocking) {
    char text[2];
    text[0] = data;
    text[1] = '\0';
    debug_print_str(text, blocking);
}

void debug_print_str(char* text, char blocking) {
    if (blocking) {
        while (*text) {
            // Transmit a byte
            if (CONS_TRMTDONE != 0) {
                CONS_TXREG = *text++;
            }
        } 
    } else {
        while (*text) {
            // Transmit a byte
            *cs_write = *text++;
            if (cs_write == cs_end) {
                cs_write = cs_begin; // Increment the write pointer
            } else {
                cs_write++;
            }
        
            if (cs_write == cs_read) {
                if (cs_write == cs_begin) {
                    cs_write = cs_end;
                } else {
                    cs_write--;
                }
                *cs_write = '#';
            }
        } 
    }
}
