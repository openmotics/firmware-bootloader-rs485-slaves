#include "DataTypes.h"
#include <stdio.h>
#include <string.h>
#include "NonVolatileMemory.h"
#include "RS485.h"

enum {
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
} RECV_STATES;

unsigned8 error = 0;

#pragma udata received_data
    unsigned8 received_data[100];
#pragma udata
#pragma udata raw_send_data
    unsigned8 raw_send_data[100];
#pragma udata

unsigned8 send_data_counter = 0;
unsigned8 received_command_second;
unsigned8 received_command_first;
unsigned16 received_crc;
unsigned16 send_crc;
unsigned8 module_address[4];

void init_uart() {
    unsigned32_mask dwBaud;

    dwBaud.value = (SYSTEM_CLOCK / 4) / COMM_BAUDRATE - 1;

    BAUDCON1bits.BRG16 = 1;
    TXSTA1bits.BRGH = 1;
    SPBRG1 = dwBaud.bytes[0];
    SPBRGH1 = dwBaud.bytes[1];
    TXSTA1bits.TXEN = 1;
    TXSTA1bits.SYNC = 0;
    RCSTA1bits.SPEN = 1;
    RCSTA1bits.CREN = 1;

    ANCON0 = 0;
    ANCON1 = 0;
    RS485_TXEN_TRIS = 0;
    RS485_TXEN = 0;

    send_data_counter = 0;
}

void putch(char c) {
    // This function is responsible for sending one character out to your USART2
    // (or whatever output device you might have, such as LCD, so that you can use DBGPRINTF()
    while (!TXSTA2bits.TRMT) {}
    TXREG2 = c;
}

unsigned8 receive_data() {
    static unsigned8 state = RECV_START1;
    unsigned8 c;
    unsigned8 done = false;
    static unsigned8 counter = 0;
    static unsigned8 TotCount = 0;

    if (UART_RXOERR == 1) { // Toggling CREN should be enough to reset the receiver after an overrun
        UART_CREN = 0; //UART_RXOERR = 0;
        _delay(100);
        UART_CREN = 1;
        return false;
    }

    if (UART_RXFERR == 1) {
        c = UART_RXREG;
        return false;
    }

    if(UART_RXFLAG == 1) {
        c = UART_RXREG;
        UART_RXFLAG = 0;

        switch(state) {
            case RECV_START1:
                if (c == 'S') {
                    state++;
                }
                break;
            case RECV_START2:
                if (c == 'T') {
                    state = RECV_ADDR1;
                } else {
                    state = RECV_START1;
                }
                break;
            case RECV_ADDR1:
                received_crc = c;
                if (c == module_address[0]) {
                    state = RECV_ADDR2;
                } else {
                    state = RECV_START1;
                }
                break;
            case RECV_ADDR2:
                received_crc += c;
                if (c == module_address[1]) {
                    state = RECV_ADDR3;
                } else {
                    state = RECV_START1;
                }
                break;
            case RECV_ADDR3:
                received_crc += c;
                if (c == module_address[2]) {
                    state = RECV_ADDR4;
                } else {
                    state = RECV_START1;
                }
                break;
            case RECV_ADDR4:
                received_crc += c;
                if (c == module_address[3]) {
                    state = RECV_CMD1;
                } else {
                    state = RECV_START1;
                }
                break;
            case RECV_CMD1:
                received_crc += c;
                received_command_first = c;
                state = RECV_CMD2;
                break;
            case RECV_CMD2:
                received_crc += c;
                received_command_second = c;
                if (received_command_first != 'F') {
                    state = WAIT_TILL_RECV_RESET;
                    break;
                }
                switch (received_command_second) {
                    case 'N':
                        counter = 0;
                        TotCount = 6;
                        state = RECV_DATA;
                        break; 
                    case 'C':
                        counter = 0;
                        TotCount = 7;
                        state = RECV_DATA;
                        break;
                    case 'D':
                        counter = 0;
                        TotCount = 69;
                        state = RECV_DATA;
                        break;
                    case 'E':
                    case 'R':
                    case 'V':
                    case 'G':
                        counter = 0;
                        TotCount = 3; // Only load crc
                        state = RECV_DATA;
                        break;
                    default:
                        state = WAIT_TILL_RECV_RESET;
                        break;
                }
                break;
            case RECV_DATA:
                received_data[counter++] = c;
                if (counter < TotCount) {
                    break;
                }
                state = WAIT_TILL_RECV_RESET;
                break;
            case WAIT_TILL_RECV_RESET:
                if (c == '\r') {
                    state = WAIT_TILL_RECV_RESET4; // Todo
                }
                break;
            case WAIT_TILL_RECV_RESET2:
                if (c == '\n') {
                    state = WAIT_TILL_RECV_RESET3;
                } else {
                    state = WAIT_TILL_RECV_RESET;
                }
                break;
            case WAIT_TILL_RECV_RESET3:
                if (c == '\r') {
                    state = WAIT_TILL_RECV_RESET4;
                } else {
                    state = WAIT_TILL_RECV_RESET;
                }
                break;
            case WAIT_TILL_RECV_RESET4:
                if (c == '\n') {
                    state = RECV_START1;
                    done = true;
                } else {
                    state = WAIT_TILL_RECV_RESET;
                }
                break;
            default:
                state = RECV_START1;
                break;
        }
    }
    return done;
}

void PutCh (unsigned8 ch) {
    while(!UART_TRMTDONE);
    UART_TXREG = ch;
    send_crc += ch;
}

void send_data() {
    unsigned8 i = 0;
    unsigned16_mask crc;
    unsigned8 doubleloop = 0;

    for (doubleloop = 0; doubleloop < 2; doubleloop++) {
        UART_CREN = 0;
        RS485_TXEN = 1;

        PutCh('R');
        PutCh('C');
        send_crc = 0;
        PutCh(module_address[0]);
        PutCh(module_address[1]);
        PutCh(module_address[2]);
        PutCh(module_address[3]);
        PutCh('F');
        PutCh(received_command_second);
        PutCh(error);

        while (i < send_data_counter) {
            PutCh(raw_send_data[i++]);
        }

        crc.value = send_crc;

        PutCh('C');
        PutCh(crc.MSB);
        PutCh(crc.LSB);
        PutCh('\r');
        PutCh('\n');
        PutCh('\r');
        PutCh('\n');

        while(!UART_TRMTDONE) {}
        RS485_TXEN = 0;
        UART_CREN = 1;
        if (received_command_second != 'E') { // If not E, stop otherwise do a double loop
            doubleloop = 2;
        }
    }
    send_data_counter = 0;
}
