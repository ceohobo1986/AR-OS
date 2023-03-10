/*
    Copyright ? 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH0(void, ColdReboot,
 
    SYNOPSIS
 
    LOCATION
 	struct ExecBase *, SysBase, 121, Exec)
 
    FUNCTION
	Reboots AROS.
 
    INPUTS
 
    RESULT
 
    NOTES
	This function never returns.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

	#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(ColdReboot,Exec,121)
AROS_SLIB_ENTRY(ColdReboot,Exec,121):
	jsr	Disable(%a6)			/* disable */
	moveq.l	#0,%d0				/* cache bits */
	moveq.l	#-1,%d1				/* cache mask */
	jsr	CacheControl(%a6)		/* disable all cache modes */

	lea.l	resetus(%pc),%a5	/* actual reset routine */
	jsr	Supervisor(%a6)			/* supervisor mode, ofcourse */

	.balign 4
resetus:
/*
//warning FIXME! This code won't work!
	lea.l	initial_ssp,%a0	/* pointer to initial ssp */
	move.l	(%a0),%d0		/* the initial ssp itself */
	move.l	4(%a0),%a0		/* get vector from initial_ssp+4 */
	reset					/* reset stuff */
	move.l	%d0,%ssp		/* load ssp with initial value -
								hopefully I am in supervisor
								mode after reset - 
								didn't read the manual... */
	jmp	(%a0)				/* this instruction was still in prefetch */
	/* never gets here */
*/
