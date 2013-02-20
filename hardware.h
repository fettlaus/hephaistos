// (c) 2010-2012 nerdfever.com

#ifndef _HARDWARE_PROFILE_H
#define _HARDWARE_PROFILE_H

#if !defined(__C32__)
	#define __C32__
#endif

#include <plib.h>

#define HARDWARE_SPI							// vs. software bit-bang (slower)

#define BYTEPTR(x)			((UINT8*)&(x))		// converts x to a UINT8* for bytewise access ala x[foo]
   	
#define FOSC				(20000000)			// PIC32 cpu clock speed, Hz
#define BAUD_RATE			(460800)			// bits per second (UART2: 8 bits, no parity, 1 stop bit)

#define PBCLK (FOSC)
#define BAUD_VALUE(bps)		((PBCLK/4./bps)-0.5)// value for baud rate generator (rounds to closest value)

#define ONE_SECOND (FOSC/2)						// 1s of PIC32 core timer ticks (== Hz)
#define MS_TO_CORE_TICKS(x) ((UINT64)(x)*ONE_SECOND/1000)
#define CT_TICKS_SINCE(tick) (ReadCoreTimer() - (tick))								// number of core timer ticks since "tick"

// Transceiver Configuration

#define MAX_SPI_CLK_FREQ           	(10e6)		// Seems to match Table 5-5 of MRF24J20 datasheet

#define RFIF            	IFS0bits.INT4IF		// interrupt input to PIC32
#define RFIE            	IEC0bits.INT4IE
#define RF_INT_PIN      	PORTDbits.RD11  	// INT pin on RF module
#define RF_INT_TRIS     	TRISDbits.TRISD11

#define RADIO_CS            LATFbits.LATF0	
#define RADIO_CS_TRIS       TRISFbits.TRISF0
#define RADIO_RESETn        LATEbits.LATE0		// Not needed; leave floating
#define RADIO_RESETn_TRIS   TRISEbits.TRISE0
#define RADIO_WAKE        	LATFbits.LATF1		// Not needed; leave floating
#define RADIO_WAKE_TRIS   	TRISFbits.TRISF1	

#define SPI_SDI             PORTGbits.RG7		
#define SDI_TRIS            TRISGbits.TRISG7	
#define SPI_SDO             LATGbits.LATG8 		
#define SDO_TRIS            TRISGbits.TRISG8	
#define SPI_SCK             LATGbits.LATG6		
#define SCK_TRIS            TRISGbits.TRISG6	

#define SPEEDO				(LATFbits.LATF3)	// "speedometer" debug output pin

void BoardInit(void);
unsigned char ReadUART(void);

#endif	// _HARDWARE_PROFILE_H
