/*
 * Olimex STM32-H152 has two LEDs connected to GPIO port E
 * and this program demostrates how to blink green LED on it.
 */

/*
 * Register definitions
 */
#define RCC_BASE 0x40023800		/* Reset and Clock Control registers */
#define RCC_AHBENR (RCC_BASE + 0x1C)	/* AHB peripheral clock enable */
#define RCC_ICSCR (RCC_BASE + 0x04)	/* Internal Clock Sources Calibration */

#define SYST_BASE 0xE000E010		/* SysTick registers  */
#define SYST_CSR (SYST_BASE + 0x00)	/* Control and Status */
#define SYST_RVR (SYST_BASE + 0x04)	/* Reload Value */
#define SYST_CVR (SYST_BASE + 0x08)	/* Current Value */

#define GPIOE_BASE 0x40021000		/* General Purpose I/O registers */
#define GPIOE_MODER (GPIOE_BASE + 0x00)	/* Port Mode */
#define GPIOE_ODR (GPIOE_BASE + 0x14)	/* Output Data */

/* SysTick IRQ handler called every 1ms */
void systick_handler(void) __attribute__ ((interrupt ("IRQ")));

/* Millisecond resolution delay function */
void delay(int);

/* Incremented from SysTick IRQ handler every 1ms */
volatile unsigned int counter = 0;


int main(void)
{
	int *rcc_ahbenr  = ((int *)RCC_AHBENR);
	int *rcc_icscr   = ((int *)RCC_ICSCR);
	int *syst_rvr    = ((int *)SYST_RVR);
	int *syst_cvr    = ((int *)SYST_CVR);
	int *syst_csr    = ((int *)SYST_CSR);
	int *gpioe_odr   = ((int *)GPIOE_ODR);
	int *gpioe_moder = ((int *)GPIOE_MODER);

	/* Study ST's Reference Manual RM0038 for register definitions */

	/* Configure core to use 4MHz MSI clock */
	*rcc_icscr = (*rcc_icscr | (0x6<<13));

	/* Set up SysTick to interrupt every 1ms */
	*syst_rvr = 4000;
	*syst_cvr = 0xFFFFFFFF;
	*syst_csr = 0x7;

	/* Enable GPIOE clock and configure output */
	*rcc_ahbenr = (*rcc_ahbenr | 1<<4);
        *gpioe_odr = (*gpioe_odr | 1<<11); /* LED off by default */
        *gpioe_moder = (*gpioe_moder | 1<<22);

	while(1)
	{
            delay(500);
            /* LED on */
            *gpioe_odr = (*gpioe_odr & ~(1<<11));
	    delay(500);
            /* LED off */
            *gpioe_odr = (*gpioe_odr | 1<<11);
	}

        return 0;
}

void systick_handler(void)
{
        counter++;
}

void delay(int ms)
{
	int stop;
	stop = counter + ms;

	while(counter != stop);
}
