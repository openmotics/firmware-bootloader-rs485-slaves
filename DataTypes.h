#include <xc.h>

#ifndef __DATA_TYPES_H
    #define __DATA_TYPES_H

    typedef unsigned char unsigned8;
    typedef unsigned short int unsigned16;
    typedef signed int signed16;
    typedef unsigned long unsigned32;
    typedef signed long signed32;
    typedef uint24_t unsigned24;
    typedef enum _t_boolean { false = 0, true = 1 } boolean;

    typedef union _t_ureg32 {
        unsigned32 value;
        struct {
            unsigned16 LW;
            unsigned16 HW;
        };
        unsigned8 bytes[4];
    } ureg32;

    typedef union _t_unsigned8_masks {
        unsigned8 value;
        struct {
            unsigned int b0:1;
            unsigned int b1:1;
            unsigned int b2:1;
            unsigned int b3:1;
            unsigned int b4:1;
            unsigned int b5:1;
            unsigned int b6:1;
            unsigned int b7:1;
        } bits;
    } unsigned8_mask;

    typedef union _t_unsigned16_masks {
        unsigned16 value;
        struct {
            unsigned8 LSB;
            unsigned8 MSB;
        };
        unsigned8 bytes[2];
    } unsigned16_mask;

    typedef union _t_unsigned32_masks {
        unsigned32 value;
        unsigned16 words[2];
        unsigned8 bytes[4];
        struct {
            unsigned16 LW;
            unsigned16 HW;
        } word;
        struct {
            unsigned8 LB;
            unsigned8 HB;
            unsigned8 UB;
            unsigned8 MB;
        } byte;
        struct {
            unsigned char b0:1;
            unsigned char b1:1;
            unsigned char b2:1;
            unsigned char b3:1;
            unsigned char b4:1;
            unsigned char b5:1;
            unsigned char b6:1;
            unsigned char b7:1;
            unsigned char b8:1;
            unsigned char b9:1;
            unsigned char b10:1;
            unsigned char b11:1;
            unsigned char b12:1;
            unsigned char b13:1;
            unsigned char b14:1;
            unsigned char b15:1;
            unsigned char b16:1;
            unsigned char b17:1;
            unsigned char b18:1;
            unsigned char b19:1;
            unsigned char b20:1;
            unsigned char b21:1;
            unsigned char b22:1;
            unsigned char b23:1;
            unsigned char b24:1;
            unsigned char b25:1;
            unsigned char b26:1;
            unsigned char b27:1;
            unsigned char b28:1;
            unsigned char b29:1;
            unsigned char b30:1;
            unsigned char b31:1;
        } bits;
    } unsigned32_mask;

    #define LSB(a) ((a).v[0])
    #define MSB(a) ((a).v[1])
    #define TRUE 1
    #define FALSE 0
#endif
