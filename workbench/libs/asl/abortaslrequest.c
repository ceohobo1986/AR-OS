/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.

    Desc:
*/


#include <proto/exec.h>
#include "asl_intern.h"

#include <string.h>

/*****************************************************************************

    NAME */

        AROS_LH1(void, AbortAslRequest,

/*  SYNOPSIS */
        AROS_LHA(APTR, requester, A0),

/*  LOCATION */
        struct Library *, AslBase, 13, Asl)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ReqNode *reqnode;

    if (!requester) return;

    ObtainSemaphore( &(ASLB(AslBase)->ReqListSem));

    reqnode = FindReqNode(requester, ASLB(AslBase));
    if (reqnode)
    {
        if (reqnode->rn_ReqWindow)
        {
#ifdef __MORPHOS__
            /* NOTE: This is asyncron, but so is the original. - Piru */
            WindowAction(reqnode->rn_ReqWindow, WAC_SENDIDCMPCLOSE, 0);
#else

            struct MsgPort      mp;
            struct IntuiMessage msg;
            BYTE                sig;

            sig = AllocSignal(-1);
            if (sig == -1) sig = SIGB_SINGLE;

            memset( &mp, 0, sizeof( mp ) );
            mp.mp_Node.ln_Type = NT_MSGPORT;
            mp.mp_Flags        = PA_SIGNAL;
            mp.mp_SigTask      = FindTask(NULL);
            mp.mp_SigBit       = sig;
            NEWLIST(&mp.mp_MsgList);

            msg.ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
            msg.ExecMessage.mn_ReplyPort    = &mp;
            msg.ExecMessage.mn_Length       = sizeof(msg);
            msg.Class                       = IDCMP_CLOSEWINDOW;
            msg.Code                        = 0;
            msg.Qualifier                   = 0;
            msg.IAddress                    = reqnode->rn_ReqWindow;
            msg.MouseX                      = 0;
            msg.MouseY                      = 0;
            msg.Seconds                     = 0;
            msg.Micros                      = 0;
            msg.IDCMPWindow                 = reqnode->rn_ReqWindow;
            msg.SpecialLink                 = 0;

            SetSignal(0, 1L << sig);
            PutMsg(reqnode->rn_ReqWindow->UserPort, &msg.ExecMessage);
            /* Release the semaphore lock to avoid deadlock - Piru */
            ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));
            WaitPort(&mp);

            if (sig != SIGB_SINGLE) FreeSignal(sig);

            return;
#endif
        } /* if (reqnode->rn_ReqWindow) */

    } /* if (reqnode) */

    ReleaseSemaphore(&(ASLB(AslBase)->ReqListSem));


    AROS_LIBFUNC_EXIT

} /* AbortAslRequest */
