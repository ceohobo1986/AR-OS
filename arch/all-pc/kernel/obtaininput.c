/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/obtaininput.c for documentation */

AROS_LH0I(int, KrnObtainInput,
        struct KernelBase *, KernelBase, 33, Kernel)
{
    AROS_LIBFUNC_INIT

    return TRUE;

    AROS_LIBFUNC_EXIT
}
