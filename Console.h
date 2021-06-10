#ifndef __CONSOLE_H
    #define __CONSOLE_H

    #include <stdio.h>
    #include "DataTypes.h"

    #define CONS_BAUDRATE 115200
    #define CONSOLE_STREAM_SIZE 1

    void init_debug_uart(void);
    void putch(char c);
#endif
