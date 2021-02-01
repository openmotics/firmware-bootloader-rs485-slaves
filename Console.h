#ifndef __CONSOLE_H
    #define __CONSOLE_H

    #include <stdio.h>
    #include "DataTypes.h"

    #define CONS_BAUDRATE 115200
    #define CONSOLE_STREAM_SIZE 1

    void debug_print_str(char* text,char blocking);
    void debug_print_int(unsigned data,char blocking);
    void debug_print_byte(unsigned8 data,char blocking);
    void debug_print_chr(unsigned8 data, char blocking);
    void debug_print_long(unsigned32 data,char blocking);
    void init_debug_uart(void);
#endif
