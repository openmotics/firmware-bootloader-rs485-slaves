#ifndef __NONVOLATILEMEMORY_H
    #define __NONVOLATILEMEMORY_H

    #include "ModuleType.h"
    #include "DataTypes.h"

    #define BLOCK_SIZE 0x40 // 64 byte programming block size on the PIC18Fx7K22 family devices
    #define PROG_NUM_OF_BLOCKS 0x39A

    void erase_program_memory(unsigned16 page);
    void write_program_memory(unsigned8* data, ureg32 source_address);
    void read_program_memory(unsigned8* data, ureg32 source_address, unsigned16 Num);

    #ifndef __DESIGNSTANDARDS_H
        #define __DESIGNSTANDARDS_H

        #include <stdio.h>

        #define SYSTEM_CLOCK 40000000ul
        #define INSTRUCTION_CLOCK SYSTEM_CLOCK / 4
        #define PERIPHERAL_CLOCK INSTRUCTION_CLOCK
        #define STEP_SIZE INSTRUCTION_CLOCK / 10000

        #define LITTLE_ENDIAN
        #ifndef LITTLE_ENDIAN
            #define BIG_ENDIAN
        #endif
    #endif
#endif
