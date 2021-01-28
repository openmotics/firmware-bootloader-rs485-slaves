#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdio.h>
#include "DataTypes.h"

#define CONS_BAUDRATE 115200
#define CONSOLE_STREAM_SIZE 1

#define VERBOSE_DEBUG

void DBGPrintROM(const char* text,char blocking);
void Print( char* text,char blocking);
void DBGprintINT(unsigned a,char blocking);
void DBGprintBYTE(unsigned8 a,char blocking);
void DBGprintLONG(unsigned32 a,char blocking);
void InitDebugUART(void);

#endif
