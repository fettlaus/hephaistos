/*
 * Simple FreeJTAG Testfirmware
 * Author: Arne Wischer
*/

#include "ch.h"
#include "hal.h"
#include "test.h"

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
  /* re-enable interrupt after 3000mS.*/
  chVTSetI(&vt4, MS2ST(3000), enableinterrupt, NULL);
  chSysUnlockFromIsr();
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
    halInit();
    chSysInit();

    extStart(&EXTD1, &extcfg);
    uartStart(&UARTD3, &uart_cfg_1);

    extChannelEnable(&EXTD1, 9);
    while(1){
    	uartStartSend(&UARTD3,20,"Hello this is UART!\n");
        chThdSleepMilliseconds(1000);
    }
}
