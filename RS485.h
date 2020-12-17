#ifndef _RS485_H
#define _RS485_H

#include "DataTypes.h"





#define COMM_BAUDRATE 115200


#define COMM_UART_SEL 1

#define RS485_TXEN_TRIS TRISDbits.TRISD4
#define RS485_TXEN		PORTDbits.RD4


#if COMM_UART_SEL == 1
	#define UART_TRISTx		TRISCbits.TRISC6
	#define UART_TRISRx		TRISCbits.TRISC7
	#define UART_RXFLAG		PIR1bits.RC1IF
	#define UART_TXREG		TXREG1
	#define CONS_TXREG		TXREG2
	#define UART_TRMTDONE	TXSTA1bits.TRMT
	#define CONS_TRMTDONE	TXSTA2bits.TRMT
	#define UART_RXREG		RCREG1
	#define UART_RXOERR		RCSTA1bits.OERR
	#define UART_RXFERR		RCSTA1bits.FERR
	#define UART_CREN		RCSTA1bits.CREN
#elif COMM_UART_SEL == 2
	#define UART_TRISTx		TRISGbits.TRISG1
	#define UART_TRISRx		TRISGbits.TRISG2
	#define UART_RXFLAG		PIR3bits.RC2IF
	#define UART_TXREG		TXREG2
	#define CONS_TXREG		TXREG1
	#define UART_TRMTDONE	TXSTA2bits.TRMT
	#define CONS_TRMTDONE	TXSTA1bits.TRMT
	#define UART_RXREG		RCREG2
	#define UART_RXOERR		RCSTA2bits.OERR
	#define UART_RXFERR		RCSTA2bits.FERR
	#define UART_CREN		RCSTA2bits.CREN
#endif

void init_uart(void);



extern unsigned8 error;

extern unsigned8 RECV_Data[];
extern unsigned8 SendDataRaw[];
extern unsigned8 SendDataCount;
extern unsigned8 RECV_command;
extern unsigned8 RECV_comm;
extern unsigned16 Recv_crc;

extern unsigned8 ADDR[];




void SendData(void);
unsigned8 RecvData(void);

#endif
