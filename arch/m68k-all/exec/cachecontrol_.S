/*
    Copyright ? 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CacheControl() - Global control of the system caches.
    Lang: english
*/

	#include "aros/m68k/asm.h"
	#include "cache.h"

	.chip 68060

	.text

	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheControl_00,Exec,108)
	.type	AROS_SLIB_ENTRY(CacheControl_00,Exec,108),@function
AROS_SLIB_ENTRY(CacheControl_00,Exec,108):
	/* 68000/010 have no caches */

#if CACHEDEBUG
	bsr		cachedebug
#endif

	moveq #0,%d0

#if CACHEDEBUG
	bsr		cachedebug2
#endif

	rts

	.globl	AROS_SLIB_ENTRY(CacheControl_20,Exec,108)
	.type	AROS_SLIB_ENTRY(CacheControl_20,Exec,108),@function
AROS_SLIB_ENTRY(CacheControl_20,Exec,108):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea.l	cachecontrolsup_20,%a5
	jmp	Supervisor(%a6)

cachecontrolsup_20:
	move.l	%a1,%a5
	movem.l	%d2/%d3,-(%sp)
	or.w	#0x0700,%sr
	move.l	%d0,%d2
	movec	%cacr,%d0
	move.l	%d0,%d3
	and.l	%d1,%d2
	not.l	%d1
	and.l	%d1,%d3
	or.l	%d2,%d3
	or.w	#0x0808,%d3 // CACRF_ClearI|CACRF_ClearD
	movec	%d3,%cacr
	movem.l	(%sp)+,%d2/%d3

#if CACHEDEBUG
	bsr		cachedebug2
#endif

	rte

	.balign 4
	.globl	AROS_SLIB_ENTRY(CacheControl_40,Exec,108)
	.type	AROS_SLIB_ENTRY(CacheControl_40,Exec,108),@function
AROS_SLIB_ENTRY(CacheControl_40,Exec,108):

#if CACHEDEBUG
	bsr		cachedebug
#endif

	move.l	%a5,%a1
	lea.l	cachecontrolsup_40,%a5
	jmp	Supervisor(%a6)
	
cachecontrolsup_40:
	move.l	%a1,%a5
	movem.l	%d2/%d3,-(%sp)
    move.l	%d0,%d2

	and.l	#0x0101,%d1 // CACRF_EnableI|CACRF_EnableD
	and.l	#0x0101,%d2 // CACRF_EnableI|CACRF_EnableD

	// code cache 0 -> 15
    bclr	#0,%d1
    beq.s	3f
    or.l	#0x20808000,%d1
3:	bclr	#0,%d2
	beq.s	4f
    or.l	#0x20808000,%d2
4:  
    // data cache 8 -> 31
    bclr	#8,%d1
    beq.s	5f
    bset	#31,%d1
5:	bclr	#8,%d2
	beq.s	2f
	bset	#31,%d2
2:
	or.w	#0x0700,%sr
	movec	%cacr,%d0
	move.l	%d0,%d3
	and.l	%d1,%d2
	not.l	%d1
	and.l	%d1,%d3
	or.l	%d2,%d3
	// documentation says caches must be
	// flushed before modifying cache state.
	nop
	cpusha	%bc
	nop
	movec	%d3,%cacr
	
    move.l %d0,%d1
    moveq #0,%d0
	// 040/060 code cache?
    btst #15,%d1
    beq.s 1f
    // ->code+burst
    or.w #0x0011,%d0 // CACRF_IBE|CACRF_EnableI
1:	// 040/060 data cache?
	btst #31,%d1
	beq.s 0f
	// ->data+burst+copyback
	or.l #0x80001100,%d0 // CACRF_CopyBack|CACRF_DBE|CACRF_EnableE
0:
	movem.l (%sp)+,%d2/%d3

#if CACHEDEBUG
	bsr		cachedebug2
#endif

    rte

#if CACHEDEBUG
cachedebug:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d1,-(%sp)
	move.l	%d0,-(%sp)
	pea format
	jsr kprintf
	lea	12(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
cachedebug2:
	movem.l %d0-%d1/%a0-%a1,-(%sp)
	move.l	%d0,-(%sp)
	pea format2
	jsr kprintf
	lea	8(%sp),%sp
	movem.l (%sp)+,%d0-%d1/%a0-%a1
	rts
format:
	.string "CO(%08x,%08x)"
format2:
	.string "=%08x\n"
#endif

