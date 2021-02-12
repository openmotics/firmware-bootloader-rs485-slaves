#ifndef _RS485_H
    #define _RS485_H

    #include "DataTypes.h"

    #define COMM_BAUDRATE 115200
    #define RS485_TXEN_TRIS TRISDbits.TRISD4
    #define RS485_TXEN PORTDbits.RD4
    #define UART_TRISTx TRISCbits.TRISC6
    #define UART_TRISRx TRISCbits.TRISC7
    #define UART_RXFLAG PIR1bits.RC1IF
    #define UART_TXREG TXREG1
    #define CONS_TXREG TXREG2
    #define UART_TRMTDONE TXSTA1bits.TRMT
    #define CONS_TRMTDONE TXSTA2bits.TRMT
    #define UART_RXREG RCREG1
    #define UART_RXOERR RCSTA1bits.OERR
    #define UART_RXFERR RCSTA1bits.FERR
    #define UART_CREN RCSTA1bits.CREN

    extern unsigned8 error;
    extern unsigned8 received_data[];
    extern unsigned8 raw_send_data[];
    extern unsigned8 send_data_counter;
    extern unsigned8 received_command_second;
    extern unsigned8 received_command_first;
    extern unsigned16 received_crc;
    extern unsigned8 module_address[];

    void init_uart(void);
    void send_data(void);
    unsigned8 receive_data(void);
#endif
