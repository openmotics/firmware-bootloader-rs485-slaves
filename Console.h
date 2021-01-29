#ifndef __CONSOLE_H
    #define __CONSOLE_H

    #include <stdio.h>
    #include "DataTypes.h"

    #define CONS_BAUDRATE 115200
    #define CONSOLE_STREAM_SIZE 1

    void DBGPrintSTR(char* text,char blocking);
    void DBGPrintINT(unsigned a,char blocking);
    void DBGPrintBYTE(unsigned8 a,char blocking);
    void DBGPrintCHR(unsigned8 a, char blocking);
    void DBGPrintLONG(unsigned32 a,char blocking);
    void InitDebugUART(void);
#endif
