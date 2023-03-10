/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.
*/

#include <bootconsole.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"

#if defined(DEBUG_USEATOMIC)
volatile ULONG   _arosdebuglock = 1;
#endif

int krnPutC(int c, struct KernelBase *KernelBase)
{
    unsigned long flags;

    __save_flags(flags);

    /*
     * stegerg: Don't use Disable/Enable, because we want  interrupt enabled flag
     * to stay the same as it was before the Disable() call
     */
    __cli();

    /*
     * If we got 0x03, this means graphics driver wants to take over the screen.
     * If VESA hack is activated, it will use only upper half of the screen
     * because y resolution was adjusted.
     * In our turn, we need to switch over to lower half.
     * VESA hack is supported only on graphical console of course. And do not
     * expect it to work with native mode video driver. :)
     */
    if ((c == 0x03) && (scr_Type == SCR_GFX) && __KernBootPrivate->debug_framebuffer)
    {
        /* Reinitialize boot console with decreased height */
        scr_FrameBuffer = __KernBootPrivate->debug_framebuffer;
        fb_Resize(__KernBootPrivate->debug_y_resolution);
    }
    else
        con_Putc(c);

    /*
     * Interrupt flag is stored in flags - if it was enabled before,
     * it will be renabled when the flags are restored
     */
    __restore_flags(flags);

    return 1;
}
