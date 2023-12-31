	@IMPORT mapper_irq_handler

#include "../equates.h"
#include "../6502mac.h"

MAPPER_OVERLAY_TEXT(3)

	global_func mapper67init
	global_func map67_handler
@	global_func map67_IRQ_Hook

 counter = mapperdata+0
 irqen = mapperdata+4
 suntoggle = mapperdata+5
 counter_last_timestamp = mapperdata+8

@IRQ counter counts down
@When it wraps from 0000->FFFF, it triggers an IRQ and disables itself

@----------------------------------------------------------------------------
mapper67init:	@Sunsoft, Fantazy Zone 2 (J)
@----------------------------------------------------------------------------
	.word write0,write1,write2,write3

@	ldr r0,=map67_IRQ_Hook
@	str_ r0,scanlinehook

	mov pc,lr

@irq stuff
@--------
map67_handler:
	ldr_ r1,mapper_timestamp
	bl run_2
	b_long _GO

run_counter:
	@get timestamp
	ldr_ r2,cycles_to_run
0:
	sub r2,r2,cycles,asr#CYC_SHIFT
	ldr_ r1,timestamp
	add r1,r1,r2
run_2:
	add r1,r1,#12
	ldr_ r2,counter_last_timestamp
	str_ r1,counter_last_timestamp
	sub r2,r1,r2
	@r2 = elapsed cycles
	
	@divide time difference by 3
	ldr r0,=0x55555556 @1/3
	umull addy,r2,r0,r2

	ldrb_ addy,irqen
	tst addy,#0x10
	bxeq lr
	
	ldr_ r0,counter
	
	mov r0,r0,lsl#16
	subs r0,r0,r2,lsl#16
	mov r0,r0,lsr#16
	str_ r0,counter
	
	@IRQ triggers if counter<to_run
	@skip if counter>=to_run
	bcs 0f
	
	mov r0,#0
	strb_ r0,irqen  @disable IRQs after it ticks
	
	ldr r0,=mapper_irq_handler
	adrl_ addy,mapper_irq_timeout
	b_long replace_timeout_2

find_next_irq:
	ldrb_ addy,irqen
	tst addy,#0x10
	bxeq lr

	ldr_ r2,cycles_to_run
	sub r2,r2,cycles,asr#CYC_SHIFT
	ldr_ r1,timestamp
	add r1,r1,r2
	add r1,r1,#12
	str_ r1,counter_last_timestamp
	
	ldr_ r0,counter
0:
	@get countdown until wrap
	add r0,r0,#1
	adds r0,r0,r0,lsl#1
	
	add r1,r1,r0
	sub r1,r1,#12
	adr r0,map67_handler
	adrl_ addy,mapper_timeout
	b_long replace_timeout_2

@----------------------------------------------------------------------------
write0:		@8800,9800
@----------------------------------------------------------------------------
	tst addy,#0x0800
	moveq pc,lr
	tst addy,#0x1000
	beq_long chr01_
	b_long chr23_
@----------------------------------------------------------------------------
write1:		@A800-B800
@----------------------------------------------------------------------------
	tst addy,#0x0800
	moveq pc,lr
	tst addy,#0x1000
	beq_long chr45_
	b_long chr67_
@----------------------------------------------------------------------------
write2:		@C000,C800,D800
@----------------------------------------------------------------------------
	tst addy,#0x1000
	beq 0f
	ldrb_ r1,wantirq
	bic r1,r1,#IRQ_MAPPER
	strb_ r1,wantirq
	
	mov r1,#0
	strb_ r1,suntoggle
	strb_ r0,irqen
	b find_next_irq
0:
	ldrb_ r1,irqen
	tst r1,#0x10
	bne 0f
1:
	ldrb_ r1,suntoggle
	cmp r1,#0
	streqb_ r0,counter+1
	strneb_ r0,counter+0
	eor r1,r1,#1
	strb_ r1,suntoggle
	mov pc,lr
0:
	@changing IRQ settings while counter is running, shouldn't happen
	stmfd sp!,{r0,lr}
	bl run_counter
	ldmfd sp!,{r0,lr}
	b 1b
	ldrb_ r1,suntoggle
	cmp r1,#0
	streqb_ r0,counter+1
	strneb_ r0,counter+0
	eor r1,r1,#1
	strb_ r1,suntoggle
	b find_next_irq

@----------------------------------------------------------------------------
write3:		@E800,F800
@----------------------------------------------------------------------------
	tst addy,#0x0800
	moveq pc,lr
	tst addy,#0x1000
	bne_long map89AB_
	b_long mirrorKonami_

@ .align
@ .pool
@ .section .iwram, "ax", %progbits
@ .subsection 7
@ .align
@ .pool
@@----------------------------------------------------------------------------
@map67_IRQ_Hook:
@@----------------------------------------------------------------------------
@	ldrb_ r1,irqen
@	cmp r1,#0
@	beq default_scanlinehook
@
@	ldr r0,countdown
@	ldr r1,=0x71AAAA @341 * 65536 / 3
@	subs r0,r0,r1
@	str r0,countdown
@	bpl default_scanlinehook
@
@	mov r1,#0
@	strb_ r1,irqen
@	mov r0,#-1
@	str r0,countdown
@@	b irq6502
@	b CheckI
@----------------------------------------------------------------------------
	@.end
