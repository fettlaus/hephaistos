#include <stdio.h>
#include "hardware.h"

// PIC32MX440 setup

#pragma config ICESEL=ICS_PGx2					// use PGx2 pins
#pragma config OSCIOFNC=OFF						// disable clock output on CLKO
#pragma config CP=OFF, BWP=OFF, PWP=OFF			// no code protetction
#pragma config FPLLIDIV=DIV_2					// 8 Mhz FRC (or xtal) / 2 = 4 MHz
#pragma config FPLLMUL=MUL_20					// * 20 = 80 MHz (will go up to MUL_24 = 96 MHz)
#pragma config FPLLODIV=DIV_16 					// / 16  = 5 MHz
#pragma config FPBDIV=DIV_1						// Peripheral clock = FOSC
#pragma config UPLLEN=OFF, UPLLIDIV=DIV_1		// don't use USB yet
#pragma config FCKSM=CSDCMD						// disable fail-safe clock monitor
#pragma config FWDTEN=OFF, WDTPS=PS8192			// disable WDT
#pragma config FSOSCEN=OFF, IESO=OFF			// No secondary osc, no switchover

#pragma config FNOSC=FRCPLL, POSCMOD=OFF		// Internal FRC
//#pragma config FNOSC=PRIPLL, POSCMOD=XT			// External crystal

void BoardInit(void)
{
	setbuf(stdout, NULL);							// unbuffered output

    AD1PCFG = 0xffff;								// all analog pins set to digital mode

	#define _PLLODIV (2)	// 2 = 20 MHz
	mSysUnlockOpLock(OSCCONbits.PLLODIV = _PLLODIV); 	// sets system clock frequency

 	SYSTEMConfig(FOSC, SYS_CFG_ALL);				// set for max hardware speed

	mJTAGPortEnable(0);                      		// disable JTAG

	LATB  = 0b00000000;			// clear all output bits
	LATC  = 0b00000000;			// clear all output bits
	LATD  = 0b00000000;			// clear all output bits
	LATE  = 0b00000000;			// clear all output bits
	LATF  = 0b00000000;			// clear all output bits
	LATG  = 0b00000000;			// clear all output bits

	TRISB = 0b1111111111111111;
	TRISC = 0b1111111111111111;
	TRISD = 0b1111111101111111;	// RD7=GPS_ENABLE
	TRISE = 0b1111111101100000;	// RE7=SPEEDO RE4=LEDG RE3=LEDY RE2=LEDR RE1=PIEZO RE0=RADIO_RESET
	TRISF = 0b1111111111011110;	// RF0=RADIO_CS RG1=RADIO_P RF5=U2TX RF1=RADIO_WAKE
	TRISG = 0b1111111010111111;	// RG8=SDO2 RG6=SCK2 RG9=KNOB IN

	// UART2 used for debugging and I/O

	U2BRG  = BAUD_VALUE(BAUD_RATE);
	U2STA  =    0b1010000000000;		// enable TX and RX pins
	U2MODE = 0b1000000000001000;		// on, 4x baud clock, 8 bits, no parity, 1 stop

	// setup I/O for RF module

	RADIO_CS_TRIS = 0;
	RADIO_CS = 1;						// deselect chip for now
	RADIO_RESETn_TRIS = 0;
	RADIO_RESETn = 1;					// can be just tied high

	RF_INT_TRIS = 1;

	SDI_TRIS = 1;
	SDO_TRIS = 0;
	SCK_TRIS = 0;
	SPI_SDO = 0;
	SPI_SCK = 0;

	RADIO_WAKE_TRIS = 0;
	RADIO_WAKE = 1;						// can be just tied high

	#ifdef HARDWARE_SPI

		unsigned int pbFreq;
		int SPI_Brg = -1;				// all 1s, 9 bits wide
		
		SPI2CON = 0x00008120;											// SP2 setup: enable, CKE, Master, normal mode
		pbFreq = (UINT32)FOSC / (1 << mOSCGetPBDIV() );					// Peripheral Bus Frequency = System Clock / PB Divider
	
		while (pbFreq/(2*(++SPI_Brg+1)) > MAX_SPI_CLK_FREQ);			// SPI clock rate per PIC32 datasheet, section 23.3.7
	
		SPI2BRG=SPI_Brg;												// set SPI baud rate

	#endif  // HARDWARE_SPI

	/* Set the Interrupt Priority */
	mINT4SetIntPriority(4);	

	/* Set Interrupt Subpriority Bits for INT1 */
	mINT4SetIntSubPriority(2);	

	/* Set INT1 to falling edge */
	mINT4SetEdgeMode(0);

	/* Enable INT1 */
	mINT4IntEnable(1);

	/* Enable Multi Vectored Interrupts */
	INTEnableSystemMultiVectoredInt();

	RFIF = 0;

	if( RF_INT_PIN == 0 )
		RFIF = 1;
}

unsigned char ReadUART(void)			// returns byte or 0 if no byte available.
{
	if (U2STAbits.OERR)					// if we got an overrun
		U2STAbits.OERR = 0;				// reset overrun flag

	if (U2STAbits.URXDA)				// if data available
		return U2RXREG;

	return 0;							// no RX data available
}

// configure stdout to print to UART2

void _mon_putc(char c)
{
	if (c == '\n')		// if newline, output CR LF
		putcUART2('\r');

	putcUART2(c);
}

