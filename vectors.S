/* Assemble ARM thumb instructions */
.thumb
.syntax unified

/* Vector entries are addresses pointing to actual exception handlers.
 * The first entry is the initial value of main stack pointer (MSP)
 */

.section ".vectors", "a" @note

/* Cortex-M3 system exceptions */

.long _msp		/* Initial main stack pointer (MSP)		*/
			/* Ex#/Pri Function				*/
.long reset_handler	/*   1/-3  Reset				*/
.long nmi_handler	/*   2/-2  NMI					*/
.long hardfault_handler	/*   3/-1  Hard fault				*/
.long memfault_handler	/*   4/set Memory management fault		*/
.long busfault_handler	/*   5/set Bus fault				*/
.long usefault_handler	/*   6/set Usage fault				*/
.long 0			/*   7/set Reserved				*/
.long 0			/*   8/set Reserved				*/
.long 0			/*   9/set Reserved				*/
.long 0			/*  10/set Reserved				*/
.long svc_handler	/*  11/set SVCall				*/
.long debug_handler	/*  12/set Debug monitor			*/
.long 0			/*  13/set Reserved				*/
.long pendsv_handler	/*  14/set PendSV				*/
.long systick_handler	/*  15/set SysTick				*/

/* External Interrupts */

/* .long stm32_specific_handler1 */
/* .long stm32_specific_handler2 ... */

.section ".text"

.thumb_func
.global reset_handler
reset_handler:

	/* Relocate .data section from FLASH to SRAM */
	src	.req r0
	dst	.req r1
	count	.req r3
	tmp	.req r4

	ldr	src, =_rodata_end
	ldr	dst, =_data_start
	ldr	tmp, =_data_end
	sub	count, tmp, dst
	ror	count, count, #2 /* load and store 4 addresses */
	b	2f		 /* at the time, so count/4    */
1:	ldr	tmp, [src], #4
	str	tmp, [dst], #4
2:	subs	count, #1
	bge	1b

        /* Initialize .bss section to zero */
        ldr     dst, =_bss_start
        ldr     src, =_bss_end
        sub     count, src, dst
        ror     count, count, #2
        mov     tmp, #0
        b       2f
1:      str     tmp, [dst], #4
2:      subs    count, #1
        bge     1b

        .unreq  src
        .unreq  dst
        .unreq  count
        .unreq  tmp

	/* Execute C code in privileged thread mode */
	bl	main
	b	. /* loop here if main ever returns */

.thumb_func
debug_handler:
.thumb_func
nmi_handler:
.thumb_func
hardfault_handler:
.thumb_func
memfault_handler:
.thumb_func
busfault_handler:
.thumb_func
usefault_handler:
.thumb_func
svc_handler:
.thumb_func
pendsv_handler:
	b	.
