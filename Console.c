#include "Console.h"
#include "RS485.h"
#include "NonVolatileMemory.h"

char ConsoleStream[CONSOLE_STREAM_SIZE];

char* CS_begin = &ConsoleStream[0];
char* CS_end = &ConsoleStream[CONSOLE_STREAM_SIZE-1];
char* CS_read = &ConsoleStream[0];
char* CS_write = &ConsoleStream[0];

#define DONT_PRINT_DEBUG 255
unsigned8 verbose = 1;

void SetVerbose(unsigned8 verb) {
    verbose = verb;
}

#ifndef VERBOSE_DEBUG

void InitDebugUART() {
}

#else

void InitDebugUART(){
    unsigned32_M dwBaud;
    dwBaud.Val = (SYSTEM_CLOCK/4)/CONS_BAUDRATE-1;

    #if (COMM_UART_SEL!=1)
        BAUDCON1bits.BRG16 = 1;
        TXSTA1bits.BRGH = 1;
        SPBRG1 = dwBaud.v[0];
        SPBRGH1 = dwBaud.v[1];
        TXSTA1bits.TXEN = 1;
        TXSTA1bits.SYNC = 0;
        RCSTA1bits.SPEN = 1;
        RCSTA1bits.CREN = 1;
    #else
        BAUDCON2bits.BRG16 = 1;
        TXSTA2bits.BRGH = 1;
        SPBRG2 = dwBaud.v[0];
        SPBRGH2 = dwBaud.v[1];    
        TXSTA2bits.TXEN = 1;
        TXSTA2bits.SYNC = 0;
        RCSTA2bits.SPEN = 1;
        RCSTA2bits.CREN = 1;
    #endif
}

#endif

void DBGPrintBYTE(unsigned8 a,char blocking){
    char str[4];
    sprintf(str, "%d", a);
    Print(str, blocking);
}

void DBGPrintLONG(unsigned32 a,char blocking) {
    char str[12];
    sprintf(str, "%d", a);
    Print(str, blocking);
}

void DBGPrintCHR(unsigned8 a, char blocking) {
    char str[2];
    str[0] = a;
    str[1] = '\0';
    Print(str, blocking);
}

void DBGPrintROM(const char* text,char blocking) {
    const char* ptr = text;
    if (verbose == DONT_PRINT_DEBUG) {
        return;
    }
    if (blocking) {
        while ( *ptr) {
            // Transmit a byte
            if (CONS_TRMTDONE != 0) {
                CONS_TXREG = *ptr++;
            }
        } 
    } else {
        while (*ptr) {
            // Transmit a byte
            *CS_write = *ptr++;
            if (CS_write == CS_end) {
                CS_write = CS_begin; // Increment the write pointer
            } else {
                CS_write++;
            }
        
            if (CS_write == CS_read) {
                if (CS_write == CS_begin) {
                    CS_write = CS_end;
                } else {
                    CS_write--;
                }
                *CS_write = '#';
            }
        } 
    }
}

void Print(char* ptr,char blocking){
    if (verbose == DONT_PRINT_DEBUG) {
        return;
    }
    if (blocking) {
        while ( *ptr) {
            // Transmit a byte
            if (CONS_TRMTDONE != 0) {
                CONS_TXREG = *ptr++;
            }
        } 
    } else {
        while( *ptr) {
            // Transmit a byte
            *CS_write = *ptr++;
            if (CS_write == CS_end) {
                CS_write = CS_begin; // Increment the write pointer
            } else { 
                CS_write++;
            }
        
            if (CS_write == CS_read) {
                if (CS_write == CS_begin) {
                    CS_write = CS_end;
                } else {
                    CS_write--;
                }
                *CS_write='#';
            }
        } 
    }
}
