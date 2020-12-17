#include "Console.h"
#include "RS485.h"
#include "NonVolatileMemory.h"

char ConsoleStream[CONSOLE_STREAM_SIZE];

char* CS_begin 	= &ConsoleStream[0];
char* CS_end 	= &ConsoleStream[CONSOLE_STREAM_SIZE-1];

char* CS_read 	= &ConsoleStream[0];
char* CS_write 	= &ConsoleStream[0];

#define DONT_PRINT_DEBUG 255
unsigned8 verbose = 1;

void SetVerbose(unsigned8 verb){
	verbose = verb;
}

#ifndef VERBOSE_DEBUG

void InitDebugUART(){
}

void ConsoleTask(){
}

#else

void InitDebugUART(){

	unsigned32_M dwBaud;


	dwBaud.Val = (SYSTEM_CLOCK/4)/CONS_BAUDRATE-1;

	#if (COMM_UART_SEL!=1)

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
}


void ConsoleTask(){
	if(CS_write==CS_read)return;
	if(CONS_TRMTDONE!=0){
		CONS_TXREG=*CS_read;
		if(CS_read==CS_end) CS_read=CS_begin;		
		else CS_read++;
	}
}
#endif


void DBGprintINT(unsigned a,char blocking){
/*
	char val[6];
	char i=0;
	val[i]=(a/10000)+48; 
	if(val[i]>='0')i++;
	val[i]=(a%10000)/1000+48; 
	if(val[i]>='0')i++;
	val[i]=(a%1000)/100+48; 
	if(val[i]>='0')i++;
	val[i]=(a%100)/10+48; 
	if(val[i]>='0')i++;
	val[i]=(a%10)+48;
	i++;
	val[i]='\0';
	Print(val,blocking);
*/
}

void DBGprintBYTE(unsigned8 a,char blocking){

	char val[4];
	char i=0;
	val[i]=(a/100)+48; 
	if(val[i]>='0')i++;
	val[i]=(a%100)/10+48; 
	if(val[i]>='0')i++;
	val[i]=(a%10)+48;
	i++;
	val[i]='\0';
	Print(val,blocking);
}



void DBGprintLONG(uReg32 a,char blocking){
/*
	char val[11];
	char i=0;
	char j=0;
	val[0]='0';
	val[1]='x';
	val[10]='\0';
	for(i=0;i<4;i++){
		j=a.Val[i]>>4&&0x0F;	
		if(j<10)j+='0';
		else j+=('A'-10);
		val[2+i*2];
		j=a.Val[i]>>4&&0x0F;	
		if(j<10)j+='0';
		else j+=('A'-10);
		val[3+i*2];
	}

	Print(val,blocking);
*/
}



void DBGPrintROM(const char* text,char blocking){
	const char* ptr = text;
	if (verbose==DONT_PRINT_DEBUG) return;
	if(blocking){
		 while( *ptr)
		{// Transmit a byte
			if(CONS_TRMTDONE!=0){
				CONS_TXREG=*ptr++;
			}
		} 
	}
	else{
		while( *ptr)
		{// Transmit a byte
			*CS_write = *ptr++;
			if(CS_write==CS_end) CS_write=CS_begin;		//increment the writepointer
			else CS_write++;
		
			if(CS_write==CS_read){
				if(CS_write==CS_begin)CS_write=CS_end;
				else CS_write--;
				*CS_write='#';
			}
		} 
	}
}

void Print(char* ptr,char blocking){
	if (verbose==DONT_PRINT_DEBUG) return;
	if(blocking){
	 	while( *ptr)
		{// Transmit a byte
			if(CONS_TRMTDONE!=0){
				CONS_TXREG=*ptr++;
			}
		} 
	}
	else{
		while( *ptr)
		{// Transmit a byte
			*CS_write = *ptr++;
			if(CS_write==CS_end) CS_write=CS_begin;		//increment the writepointer
			else CS_write++;
		
			if(CS_write==CS_read){
				if(CS_write==CS_begin)CS_write=CS_end;
				else CS_write--;
				*CS_write='#';
			}
		} 
	}
}



void FlushConsole(){
//	while(CS_write!=CS_read){ConsoleTask();}	
//	while (CONS_TRMTDONE==0){}
}

