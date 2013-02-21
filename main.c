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

static void pwmpcb(PWMDriver *pwmp);
static void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n);
static void spicb(SPIDriver *spip);

/* Total number of channels to be sampled by a single ADC operation.*/
#define ADC_GRP1_NUM_CHANNELS   2

/* Depth of the conversion buffer, channels are sampled four times each.*/
#define ADC_GRP1_BUF_DEPTH      4

/*
 * ADC samples buffer.
 */
static adcsample_t samples[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 4 samples of 2 channels, SW triggered.
 * Channels:    IN10   (48 cycles sample time)
 *              Sensor (192 cycles sample time)
 */
static const ADCConversionGroup adcgrpcfg = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  adccb,
  NULL,
  /* HW dependent part.*/
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  0,
  ADC_SMPR2_SMP_AN10(ADC_SAMPLE_48) | ADC_SMPR2_SMP_SENSOR(ADC_SAMPLE_192),
  0,
  ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),
  0,
  0,
  0,
  ADC_SQR5_SQ2_N(ADC_CHANNEL_IN10) | ADC_SQR5_SQ1_N(ADC_CHANNEL_SENSOR)
};

/*
 * PWM configuration structure.
 * Cyclic callback enabled, channels 1 and 2 enabled without callbacks,
 * the active state is a logic one.
 */
static PWMConfig pwmcfg = {
  10000,                                    /* 10kHz PWM clock frequency.   */
  10000,                                    /* PWM period 1S (in ticks).    */
  pwmpcb,
  {
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_ACTIVE_HIGH, NULL},
    {PWM_OUTPUT_DISABLED, NULL},
    {PWM_OUTPUT_DISABLED, NULL}
  },
  /* HW dependent part.*/
  0
};

/*
 * SPI configuration structure.
 * Maximum speed (12MHz), CPHA=0, CPOL=0, 16bits frames, MSb transmitted first.
 * The slave select line is the pin GPIOA_SPI1NSS on the port GPIOA.
 */
static const SPIConfig spicfg = {
  spicb,
  /* HW dependent part.*/
  GPIOB,
  12,
  SPI_CR1_DFF
};

/*
 * PWM cyclic callback.
 * A new ADC conversion is started.
 */
static void pwmpcb(PWMDriver *pwmp) {

  (void)pwmp;

  /* Starts an asynchronous ADC conversion operation, the conversion
     will be executed in parallel to the current PWM cycle and will
     terminate before the next PWM cycle.*/
  chSysLockFromIsr();
  adcStartConversionI(&ADCD1, &adcgrpcfg, samples, ADC_GRP1_BUF_DEPTH);
  chSysUnlockFromIsr();
}

/*
 * ADC end conversion callback.
 * The PWM channels are reprogrammed using the latest ADC samples.
 * The latest samples are transmitted into a single SPI transaction.
 */
void adccb(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void) buffer; (void) n;
  /* Note, only in the ADC_COMPLETE state because the ADC driver fires an
     intermediate callback when the buffer is half full.*/
  if (adcp->state == ADC_COMPLETE) {
    adcsample_t avg_ch1, avg_ch2;

    /* Calculates the average values from the ADC samples.*/
    avg_ch1 = (samples[0] + samples[2] + samples[4] + samples[6]) / 4;
    avg_ch2 = (samples[1] + samples[3] + samples[5] + samples[7]) / 4;

    chSysLockFromIsr();

    /* Changes the channels pulse width, the change will be effective
       starting from the next cycle.*/
    pwmEnableChannelI(&PWMD4, 0, PWM_FRACTION_TO_WIDTH(&PWMD4, 4096, avg_ch1));
    pwmEnableChannelI(&PWMD4, 1, PWM_FRACTION_TO_WIDTH(&PWMD4, 4096, avg_ch2));

    /* SPI slave selection and transmission start.*/
    spiSelectI(&SPID1);
    spiStartSendI(&SPID1, ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH, samples);

    chSysUnlockFromIsr();
  }
}

/*
 * SPI end transfer callback.
 */
static void spicb(SPIDriver *spip) {

  /* On transfer end just releases the slave select line.*/
  chSysLockFromIsr();
  spiUnselectI(spip);
  chSysUnlockFromIsr();
}

/*
 * This is a periodic thread that does absolutely nothing except increasing
 * a seconds counter.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {
  static uint32_t seconds_counter;

  (void)arg;
  chRegSetThreadName("counter");
  while (TRUE) {
    chThdSleepMilliseconds(1000);
    seconds_counter++;
  }
  return 0;
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 1 using the driver default configuration.
   * PA9 and PA10 are routed to USART1.
   */
  sdStart(&SD3, NULL);

  /*
   * If the user button is pressed after the reset then the test suite is
   * executed immediately before activating the various device drivers in
   * order to not alter the benchmark scores.
   */
  if (palReadPad(GPIOA, GPIOA_BUTTON))
    TestThread(&SD3);

  /*
   * Initializes the SPI driver 2. The SPI2 signals are routed as follow:
   * PB12 - NSS.
   * PB13 - SCK.
   * PB14 - MISO.
   * PB15 - MOSI.
   */
  spiStart(&SPID1, &spicfg);
  palSetPad(GPIOB, 12);

  /*
   * Initializes the ADC driver 1 and enable the thermal sensor.
   * The pin PC0 on the port GPIOC is programmed as analog input.
   */
  adcStart(&ADCD1, NULL);
  adcSTM32EnableTSVREFE();
  palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG);

  /*
   * Initializes the PWM driver 4, routes the TIM4 outputs to the board LEDs.
   */
  pwmStart(&PWMD4, &pwmcfg);
  palSetPadMode(GPIOE, GPIOE_LED2, PAL_MODE_ALTERNATE(2));
  palSetPadMode(GPIOE, GPIOE_LED1, PAL_MODE_ALTERNATE(2));

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state, when the button is
   * pressed the test procedure is launched with output on the serial
   * driver 1.
   */
  while (TRUE) {
    if (palReadPad(GPIOA, GPIOA_BUTTON))
      TestThread(&SD3);
    chThdSleepMilliseconds(500);
  }
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

int main2(void)
{
    static uint8_t lastFrameNumber;
    BaseSequentialStream* ptr = (BaseSequentialStream *) &SD3;

    halInit();
    chSysInit();
    RadioInit();            // cold start MRF24J40 radio
    RadioInitP2P();         // setup for simple peer-to-peer communication
    /*
     * Maximum speed SPI configuration (16MHz, CPHA=0, CPOL=0, MSb first).
     */
    static const SPIConfig hs_spicfg = {
      NULL,
      GPIOB,
      12,
      0
    };
    spiStart(&SPID1, &hs_spicfg);
    chprintf(ptr,"\nDemo for MRF24J40 running.\n");
    chprintf(ptr,"Hit A, B, or C on to send a message.\n");

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
