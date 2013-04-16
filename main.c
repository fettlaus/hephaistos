/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "hardware.h"
#include "MRF24J40.h"
#include "chprintf.h"

static void spicb(SPIDriver *spip);

/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   2

/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      4


/*
 * Maximum speed SPI configuration (16MHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig spicfg = {
  NULL,
  GPIOB,
  12,
  0
};



/*
 * SPI end transfer callback.
 */
static void spicb(SPIDriver *spip) {

  /* On transfer end just releases the slave select line.*/
  chSysLockFromIsr();
  spiUnselectI(spip);
  chSysUnlockFromIsr();
}





uint8_t txPayload[TX_PAYLOAD_SIZE];       // TX payload buffer

// inits Tx structure for simple point-to-point connection between a single pair of devices who both use the same address
// after calling this, you can send packets by just filling out:
// txPayload[] with payload and
// Tx.payloadLength,
// then calling RadioTXPacket()
void RadioInitP2P(void)
{
    Tx.frameType = PACKET_TYPE_DATA;
    Tx.securityEnabled = 0;
    Tx.framePending = 0;
    Tx.ackRequest = 1;
    Tx.panIDcomp = 1;
    Tx.dstAddrMode = SHORT_ADDR_FIELD;
    Tx.frameVersion = 0;
    Tx.srcAddrMode = NO_ADDR_FIELD;
    Tx.dstPANID = RadioStatus.MyPANID;
    Tx.dstAddr = RadioStatus.MyShortAddress;
    Tx.payload = txPayload;
}

BaseSequentialStream* ptr = (BaseSequentialStream *) &UARTD3;

static void enableinterrupt(void *arg) {

  (void)arg;
  extChannelEnable(&EXTD1, 9);
}

static void extcb1(EXTDriver *extp, expchannel_t channel) {
  (void)extp;
  (void)channel;
  static VirtualTimer vt4;
  palTogglePad(GPIOE,GPIOE_LED1);
  extChannelDisable(&EXTD1, 9);
  chSysLockFromIsr();
  uartStartSendI(&UARTD3,11,"INTERRUPT!\n");
  if (chVTIsArmedI(&vt4))
    chVTResetI(&vt4);
  /* LED4 set to OFF after 200mS.*/
  chVTSetI(&vt4, MS2ST(3000), enableinterrupt, NULL);
  chSysUnlockFromIsr();
  //chprintf(ptr,"Hello from Button!\n");
}

static UARTConfig uart_cfg_1 = {
  0,
  0,
  0,
  0,
  0,
  115200,
  0,
  USART_CR2_LINEN,
  0
};

static const EXTConfig extcfg = {
  {
	{EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_FALLING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOC, extcb1},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL},
    {EXT_CH_MODE_DISABLED, NULL}
  }
};


int main(void)
{
    (void)spicb;
    static uint8_t lastFrameNumber;


    halInit();
    chSysInit();
    extStart(&EXTD1, &extcfg);
    uartStart(&UARTD3, &uart_cfg_1);
    //chprintf(ptr,"Hello, this is UART!\n");
    //TestThread(&SD3);
    extChannelEnable(&EXTD1, 9);
    while(1){

    	uartStartSend(&UARTD3,20,"Hello this is UART!\n");
        chThdSleepMilliseconds(1000);
    }
    spiStart(&SPID1, &spicfg);
    chprintf(ptr,"spi started\n");
    RadioInit();            // cold start MRF24J40 radio
    chprintf(ptr,"Radio initialized\n");
    RadioInitP2P();         // setup for simple peer-to-peer communication
    chprintf(ptr,"\nDemo for MRF24J40 running.\n");


    while(1)                // main program loop
    {
        // process any received packets

        while(RadioRXPacket())
        {
            if (Rx.frameNumber != lastFrameNumber)              // skip duplicate packets (Usually because far-end missed my ACK)
            {
                lastFrameNumber = Rx.frameNumber;

                Rx.payload[Rx.payloadLength] = 0;               // put terminating null on received payload
                chprintf(ptr,"%s", Rx.payload);                       // print payload as an ASCII string
            }
            RadioDiscardPacket();
        }

        // transmit a message if one of the keys was pressed
/*
        switch (ReadUART())             // read a byte from the terminal (0=none available)
        {
        case ('A'):
        case ('a'):
            Tx.payloadLength = sprintf(Tx.payload, "This is message A for Alpha.\n" );
            RadioTXPacket();

            break;

        case ('B'):
        case ('b'):
            Tx.payloadLength = sprintf(Tx.payload, "Message B for Bravo.\n");
            RadioTXPacket();
            break;

        case ('C'):
        case ('c'):
            Tx.payloadLength = sprintf(Tx.payload, "C is for Charlie.\n");
            RadioTXPacket();
            break;
        }
        */
    }
}
