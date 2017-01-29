#ifndef __NONVOLATILEMEMORY_H
#define __NONVOLATILEMEMORY_H

#include "ModuleType.h"

#include "Compiler.h"
#include "DataTypes.h"


#define	BLOCK_SIZE					0x40 	 //64 byte programming block size on the PIC18Fx7K22 family devices



#if defined(OUTPUT_MODULE)	
	#define PROG_NUM_OF_BLOCKS			0x19A
#endif
#if defined(INPUT_MODULE)	
	#define PROG_NUM_OF_BLOCKS			0x19A
#endif
#if defined(DIMMER_MODULE)	
	#define PROG_NUM_OF_BLOCKS			0x19A
#endif
#if defined(CAN_CONTROL_MODULE)	
	#define PROG_NUM_OF_BLOCKS			0x39A	 	
#endif


//extern unsigned32 ReadLatch(unsigned16, unsigned16);
//void EraseEEPROM(unsigned16 Page);
void Erase(unsigned16 Page);
void WritePM(unsigned8* ptrData, uReg32 SourceAddr);
void ReadPMn(unsigned8* ptrData, uReg32 SourceAddr, unsigned16 Num);
void WriteCM(unsigned8* ptrData, uReg32 SourceAddr);
#if defined(DEVICE_WITH_EEPROM)
void WriteEEPROM(unsigned8* ptrData, uReg32 SourceAddr, unsigned16 size);
#endif
//void ReadDeviceID(void);



//----------------------------------------------------------------------------------------------------------------------
// SENSOR MODULE
//----------------------------------------------------------------------------------------------------------------------

#ifndef __DESIGNSTANDARDS_H
#define __DESIGNSTANDARDS_H


#include <stdio.h>

//#define ldr_debug
//#define debug_cipher

#if defined(OUTPUT_MODULE)	
	#define GetSystemClock()		(40000000ul) 
#endif
#if defined(INPUT_MODULE)	
	#define GetSystemClock()		(40000000ul) 
#endif
#if defined(DIMMER_MODULE)	
	#define GetSystemClock()		(40000000ul) 
#endif
#if defined(CAN_CONTROL_MODULE)	
	#define GetSystemClock()		(40000000ul)	 	
#endif

#define GetInstructionClock()	(GetSystemClock()/4)
#define GetPeripheralClock()	GetInstructionClock()

void Delay100us(unsigned16 x);
void DelayMs(unsigned16 ms);

#define LITTLE_ENDIAN

#ifndef LITTLE_ENDIAN
	#define BIG_ENDIAN
#endif



/*
#ifndef VERBOSE_DEBUG
	#define DBGPRINTF(...)
#else
//	#define DBGPRINTF(format, ...)	printf(format, ##__VA_ARGS__)
    #define DBGPRINTF(...)			printf(##__VA_ARGS__)
#endif
*/

#endif

#endif
