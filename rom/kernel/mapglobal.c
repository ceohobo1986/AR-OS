/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

        AROS_LH4I(int, KrnMapGlobal,

/*  SYNOPSIS */
        AROS_LHA(void *, virt, A0),
        AROS_LHA(void *, phys, A1),
        AROS_LHA(uint32_t, length, D0),
        AROS_LHA(KRN_MapAttr, flags, D1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 16, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}
