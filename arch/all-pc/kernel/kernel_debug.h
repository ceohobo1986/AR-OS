/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc:
*/

/*
 * This file contains useful redefinition of bug() macro which uses
 * kernel.resource's own debugging facilities. Include it if you
 * need bug() in your code.
 */

#ifndef __KERNEL_DEBUG_H_
#define __KERNEL_DEBUG_H_

#include <aros/config.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <stdarg.h>

#include "kernel_intern.h"

#ifdef bug
#undef bug
#endif

int krnPutC(int chr, struct KernelBase *KernelBase);
int krnBug(const char *format, va_list args, APTR kernelBase);
void krnDisplayAlert(const char *text, struct KernelBase *KernelBase);
void krnPanic(struct KernelBase *KernelBase, const char *fmt, ...);

#define __cli()     __asm__ __volatile__("cli": : :"memory")
#define __sti()     __asm__ __volatile__("sti": : :"memory")

#if defined(DEBUG_USEATOMIC)
#include <aros/atomic.h>
#include <asm/cpu.h>

extern volatile ULONG   _arosdebuglock;

#define KRNDEBUGLOCK \
    if (_arosdebuglock & 1) \
    { \
        while (bit_test_and_set_long((ULONG*)&_arosdebuglock, 1)) { asm volatile("pause"); }; \
    }
#define KRNDEBUGUNLOCK \
    if (_arosdebuglock & 1) \
    { \
        __AROS_ATOMIC_AND_L(_arosdebuglock, ~(1 << 1)); \
    }
#else
#define KRNDEBUGLOCK
#define KRNDEBUGUNLOCK
#endif

static inline void _bug(APTR kernelBase, const char *format, ...)
{
    va_list args;
#if defined(DEBUG_USEATOMIC)
    unsigned long flags = 0;

    if (_arosdebuglock & 1)
    {
        __save_flags(flags);
        __cli();
    }
#endif
    va_start(args, format);

    /* 
     * We use internal entry here. This is done because this function can be used
     * during early boot, while KernelBase is NULL. However it's still passed,
     * just in case.
     */
    krnBug(format, args, kernelBase);

    va_end(args);
#if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        __restore_flags(flags);
    }
#endif
}

#define bug(...) _bug(KernelBase, __VA_ARGS__)
#define nbug(...) _bug(NULL, __VA_ARGS__)
#endif
