/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc: ShutdownA() - Shut down the operating system.
*/

/* Prevent 'timeval redefinition' error */
#define timeval timeval_AROS

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_intern.h"
#include "exec_util.h"

/* See rom/exec/shutdowna.c for documentation */

AROS_LH1(ULONG, ShutdownA,
    AROS_LHA(ULONG, action, D0),
    struct ExecBase *, SysBase, 173, Exec)
{
    AROS_LIBFUNC_INIT

    int exitcode;

    switch(action & SD_ACTION_MASK)
    {
    case SD_ACTION_POWEROFF:
        exitcode = 0;
        break;

    case SD_ACTION_COLDREBOOT:
        exitcode = 0x81; /* Magic value for our bootstrap */
        break;

    default:
        return 0; /* Unknown action code */
    }

    Exec_DoResetCallbacks((struct IntExecBase *)SysBase, action);

    PD(SysBase).SysIFace->exit(exitcode);
    AROS_HOST_BARRIER

    /* Just shut up the compiler, we won't return */
    return 0;

    AROS_LIBFUNC_EXIT
}
