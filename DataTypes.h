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
    typedef enum BOOLEAN { FALSE = 0, TRUE } BOOLEAN;
    typedef unsigned8 BOOL;

    typedef union _t_uReg32 {
        unsigned32 Val32;
        struct {
            unsigned16 LW;
            unsigned16 HW;
        };
        unsigned8 Val[4];
    } uReg32;

    typedef union _t_unsigned8_Masks {
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
        unsigned8 Val;
    } unsigned8_M;

    typedef union _t_unsigned16_Masks {
        unsigned16 Val;
        struct {
            unsigned8 LSB;
            unsigned8 MSB;
        };
        unsigned8 v[2];
    } unsigned16_M;

    typedef union _t_unsigned32_Masks {
        unsigned32 Val;
        unsigned16 w[2];
        unsigned8 v[4];
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
    } unsigned32_M;

    #define LSB(a) ((a).v[0])
    #define MSB(a) ((a).v[1])
    #define FALSE 0
    #define TRUE 1
    #define true 1
    #define false 0
    #define READ 1
    #define WRITE 2
    #define OK 1
    #define NOK 0
#endif
