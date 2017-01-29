#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdio.h>
#include "DataTypes.h"

#define CONS_BAUDRATE 115200
#define CONSOLE_STREAM_SIZE	1

#define VERBOSE_DEBUG

/*
#ifndef VERBOSE_DEBUG
	#define DBGprintINT(a,blocking) 
	#define DBGprintBYTE(a,blocking) 
	#define DBGprintLONG(a,blocking) 
	#define DBGPrintROM(text,blocking) 
	#define DBGprint(ptr,blocking) 
#else
	#define DBGprintINT(a,blocking) void printINT(a,blocking)
	#define DBGprintBYTE(a,blocking) void printBYTE(a,blocking)
	#define DBGprintLONG(a,blocking) void printLONG(a,blocking)
	#define DBGPrintROM(text,blocking) void PrintROM(rom text,blocking)
	#define DBGprint(ptr,blocking) void print(ptr,blocking)
#endif
*/

void DBGPrintROM(rom char* text,char blocking);
void Print( char* text,char blocking);
void DBGprintINT(unsigned a,char blocking);
void DBGprintBYTE(unsigned8 a,char blocking);
void DBGprintLONG(uReg32 a,char blocking);
void InitDebugUART(void);
void ConsoleTask(void);
void FlushConsole(void);



//extern char text[100];

#endif
