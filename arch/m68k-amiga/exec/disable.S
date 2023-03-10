/*
    Copyright ? 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disable() - Stop interrupts from occurring.
    Lang: english
*/
/*****************************************************************************

    NAME
#include <proto/exec.h>

	AROS_LH0(void, Disable,

    LOCATION
	struct ExecBase *, SysBase, 20, Exec)

    FUNCTION
	This function will prevent interrupts from occuring. You can
	start the interrupts again with a call to Enable().

	Note that calls to Disable() nest, and for every call to
	Disable() you need a matching call to Enable().

	***** WARNING *****

	Using this function is considered very harmful, and it is
	not recommended to use this function for ** ANY ** reason.

	It is quite possible to either crash the system, or to prevent
	normal activities (disk/port i/o) from occuring.

	Note: As taskswitching is driven by the interrupts subsystem,
	      this function has the side effect of disabling
	      multitasking.

    INPUTS

    RESULT
	Interrupts will be disabled AFTER this call returns.

    NOTES
	This function preserves all registers.

	To prevent deadlocks calling Wait() in disabled state breaks
	the disable - thus interrupts may happen again.

    EXAMPLE
	No you DEFINITELY don't want to use this function.

    BUGS
	The only architecture that you can rely on the registers being
	saved is on the Motorola mc68000 family.

    SEE ALSO
	Forbid(), Permit(), Enable(), Wait()

    INTERNALS
	This function must be replaced in the $(KERNEL) or $(ARCH)
	directories in order to do some work.

******************************************************************************/
#include "aros/m68k/asm.h"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(Disable,Exec,20)

AROS_SLIB_ENTRY(Disable,Exec,20):
	/* As we said above: ALL registers must be preserved! */
	move.w	#0x4000,0xdff09a
	addq.b	#1,IDNestCnt(%a6)
	rts
