#include "DataTypes.h"
#include <stdio.h>
#include <string.h>
#include "NonVolatileMemory.h"
#include "RS485.h"

enum
{
	RECV_START1,
	RECV_START2,
	RECV_ADDR1,
	RECV_ADDR2,
	RECV_ADDR3,
	RECV_ADDR4,
	RECV_CMD1,
	RECV_CMD2,
	RECV_DATA,
	WAIT_TILL_RECV_RESET,
	WAIT_TILL_RECV_RESET2,
	WAIT_TILL_RECV_RESET3,
	WAIT_TILL_RECV_RESET4
}RECV_STATES;

unsigned8 error = 0;

#pragma udata RECV_Data
unsigned8 RECV_Data[100];
#pragma udata
#pragma udata SendDataRaw
unsigned8 SendDataRaw[100];
#pragma udata


unsigned8 SendDataCount=0;

unsigned8 RECV_command;
unsigned8 RECV_comm;
unsigned16 Recv_crc;
unsigned16 Send_crc;
unsigned8 ADDR[4];


void init_uart()
{
	unsigned32_M dwBaud;

	dwBaud.Val = (SYSTEM_CLOCK/4)/COMM_BAUDRATE-1;

	#if (COMM_UART_SEL==1)

		BAUDCON1bits.BRG16 = 1;
		TXSTA1bits.BRGH = 1;
		SPBRG1 = dwBaud.v[0];
		SPBRGH1 = dwBaud.v[1];
		TXSTA1bits.TXEN = 1;
		TXSTA1bits.SYNC = 0;
		RCSTA1bits.SPEN = 1;
		RCSTA1bits.CREN = 1;

	#else
		BAUDCON2bits.BRG16 = 1;
		TXSTA2bits.BRGH = 1;
		SPBRG2 = dwBaud.v[0];
		SPBRGH2 = dwBaud.v[1];
		TXSTA2bits.TXEN = 1;
		TXSTA2bits.SYNC = 0;
		RCSTA2bits.SPEN = 1;
		RCSTA2bits.CREN = 1;

	#endif


	ANCON0 = 0;
	ANCON1 = 0;


	RS485_TXEN_TRIS	= 0;
	RS485_TXEN		= 0;

	SendDataCount=0;

//	COMMRXWDTMR_Configure();
}

void putch(char c)
{
	// This function is responsible for sending one character out to your USART2
	// (or whatever output device you might have, such as LCD, so that you can use DBGPRINTF()
	#if COMM_UART_SEL == 1
	while(!TXSTA2bits.TRMT);
	TXREG2 = c;
	#elif COMM_UART_SEL == 2
	while(!TXSTA1bits.TRMT);
	TXREG1 = c;
	#endif
}







unsigned8 RecvData()
{
	static unsigned8 state=RECV_START1;
	unsigned8 c;
	unsigned8 done = false;
	static unsigned8 counter=0;
	static unsigned8 TotCount = 0;

	if(UART_RXOERR == 1) //toggling CREN should be enough to reset the receiver after an overrun
	{
	//	mLED_1_On();
		//DBGPRINTF("\n<COMM GetCommand> Receive overrun error! Clearing the flag and resetting...");
		UART_CREN = 0;//UART_RXOERR = 0;
		_delay(100);
		UART_CREN = 1;
	//	mLED_1_Off();
		return false;
	}

	if(UART_RXFERR == 1)
	{
	//	mLED_1_On();
		//UART_RXFERR = 0;
		c = UART_RXREG;
		//DBGPRINTF("\n<COMM GetCommand> Receive framing error!");
	//	mLED_1_Off();
		return false;
	}

	if(UART_RXFLAG == 1)
	{
		c = UART_RXREG;
		UART_RXFLAG = 0;
/*
		COMMRXWDTMR_ENABLE = 0; //toggling the enable bit of the timer resets its postscaler
		if (COMMRXWDTMR_INTERRUPTFLAG)
		{
			COMMRXWDTMR_INTERRUPTFLAG = 0;
			state = RECV_START1;
		}
		COMMRXWDTMR_VALUE = 0;
		COMMRXWDTMR_ENABLE = 1;
*/
		switch(state)
		{
			case RECV_START1:
//				RECV_command=0xFF;
				if (c=='S') state++;
				break;
			case RECV_START2:
				if(c=='T')
					state=RECV_ADDR1;
				else
					state=RECV_START1;
				break;
			case RECV_ADDR1:
				Recv_crc=c;
				if(c==ADDR[0])
					state=RECV_ADDR2;
				else
					state=RECV_START1;
				break;
			case RECV_ADDR2:
				Recv_crc+=c;
				if(c==ADDR[1])
					state=RECV_ADDR3;
				else
					state=RECV_START1;
				break;
			case RECV_ADDR3:
				Recv_crc+=c;
				if(c==ADDR[2])
					state=RECV_ADDR4;
				else
					state=RECV_START1;
				break;
			case RECV_ADDR4:
				Recv_crc+=c;
				if(c==ADDR[3])
					state=RECV_CMD1;
				else
					state=RECV_START1;
				break;
			case RECV_CMD1:
				Recv_crc+=c;
				RECV_comm = c;
				state=RECV_CMD2;
				break;
			case RECV_CMD2:
				Recv_crc+=c;
				RECV_command = c;
				if(RECV_comm!='F'){
					state=WAIT_TILL_RECV_RESET;
					break;
				}
				switch(RECV_command){
					case 'N':
						counter=0;
						TotCount = 6;
						state=RECV_DATA;
						break;
					case 'C':
						counter=0;
						TotCount = 7;
						state=RECV_DATA;
						break;
					case 'D':
						counter=0;
						TotCount = 69;
						state=RECV_DATA;
						break;
					case 'E':
					case 'V':
					case 'G':
						counter=0;
						TotCount = 3;	//only load crc
						state=RECV_DATA;
						break;
					default:
						state=WAIT_TILL_RECV_RESET;
						break;
				}
				break;
			case RECV_DATA:
				RECV_Data[counter++]=c;
				if(counter<TotCount) break;
				state=WAIT_TILL_RECV_RESET;
				break;

			case WAIT_TILL_RECV_RESET:
				if(c=='\r')	state=WAIT_TILL_RECV_RESET4;	//todo
				break;
			case WAIT_TILL_RECV_RESET2:
				if(c=='\n')
					state=WAIT_TILL_RECV_RESET3;
				else
					state=WAIT_TILL_RECV_RESET;
				break;
			case WAIT_TILL_RECV_RESET3:
				if(c=='\r')
					state=WAIT_TILL_RECV_RESET4;
				else
					state=WAIT_TILL_RECV_RESET;
				break;
			case WAIT_TILL_RECV_RESET4:
				if(c=='\n'){
					state = RECV_START1;
					done=true;
				}
				else
					state=WAIT_TILL_RECV_RESET;
				break;
			default:
				state = RECV_START1;
				break;

		}
	}
 	return done;
}




void PutCh(unsigned8 ch)
{
	while(!UART_TRMTDONE);
	UART_TXREG = ch;
	Send_crc+=ch;
}

void Delay100uss(unsigned16 x)
{
	unsigned16 i;
	for (i=0; i<x; i++)
	{
		_delay(STEP_SIZE);
	}
}



void SendData()
{
	unsigned8 i=0;
	unsigned16_M crc;
	unsigned8 doubleloop=0;

	for (doubleloop=0;doubleloop<2;doubleloop++){

		UART_CREN = 0;
		RS485_TXEN	= 1;

		PutCh('R');
		PutCh('C');
		Send_crc=0;
		PutCh(ADDR[0]);
		PutCh(ADDR[1]);
		PutCh(ADDR[2]);
		PutCh(ADDR[3]);
		PutCh('F');
		PutCh(RECV_command);
		PutCh(error);


		while(i<SendDataCount){
			PutCh(SendDataRaw[i++]);
		}

		crc.Val = Send_crc;

		PutCh('C');
		PutCh(crc.MSB);
		PutCh(crc.LSB);
		PutCh('\r');
		PutCh('\n');
		PutCh('\r');
		PutCh('\n');

		while(!UART_TRMTDONE);
		RS485_TXEN	= 0;
		UART_CREN = 1;
		if(RECV_command!='E'){ //if not E, stop otherwise do a double loop
			doubleloop=2;
		}
	}
	SendDataCount=0;


}
