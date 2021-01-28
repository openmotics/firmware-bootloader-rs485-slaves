#ifndef __NONVOLATILEMEMORY_H
#define __NONVOLATILEMEMORY_H

#include "ModuleType.h"
#include "DataTypes.h"

#define BLOCK_SIZE 0x40 // 64 byte programming block size on the PIC18Fx7K22 family devices

#if defined(OUTPUT_MODULE)    
    #define PROG_NUM_OF_BLOCKS 0x19A
#endif
#if defined(INPUT_MODULE)    
    #define PROG_NUM_OF_BLOCKS 0x19A
#endif
#if defined(DIMMER_MODULE)    
    #define PROG_NUM_OF_BLOCKS 0x19A
#endif
#if defined(CAN_CONTROL_MODULE)    
    #define PROG_NUM_OF_BLOCKS 0x39A         
#endif

void Erase(unsigned16 Page);
void WritePM(unsigned8* ptrData, uReg32 SourceAddr);
void ReadPMn(unsigned8* ptrData, uReg32 SourceAddr, unsigned16 Num);
void WriteCM(unsigned8* ptrData, uReg32 SourceAddr);

#if defined(DEVICE_WITH_EEPROM)
    void WriteEEPROM(unsigned8* ptrData, uReg32 SourceAddr, unsigned16 size);
#endif

//----------------------------------------------------------------------------------------------------------------------
// SENSOR MODULE
//----------------------------------------------------------------------------------------------------------------------

#ifndef __DESIGNSTANDARDS_H
#define __DESIGNSTANDARDS_H

#include <stdio.h>

// #define ldr_debug
// #define debug_cipher

#if defined(OUTPUT_MODULE)    
    #define SYSTEM_CLOCK 40000000ul
#endif
#if defined(INPUT_MODULE)    
    #define SYSTEM_CLOCK 40000000ul
#endif
#if defined(DIMMER_MODULE)    
    #define SYSTEM_CLOCK 40000000ul
#endif
#if defined(CAN_CONTROL_MODULE)    
    #define SYSTEM_CLOCK 40000000ul
#endif

#define INSTRUCTION_CLOCK SYSTEM_CLOCK / 4
#define PERIPHERAL_CLOCK INSTRUCTION_CLOCK
#define STEP_SIZE INSTRUCTION_CLOCK / 10000

#define LITTLE_ENDIAN
#ifndef LITTLE_ENDIAN
    #define BIG_ENDIAN
#endif

#endif

#endif
