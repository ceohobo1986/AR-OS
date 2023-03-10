/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc:
*/

#ifndef _PREFS_H_
#define _PREFS_H_

#include <prefs/asl.h>
#include <dos/dos.h>

/*********************************************************************************************/

BOOL Prefs_HandleArgs(STRPTR from, BOOL use, BOOL save);
BOOL Prefs_ImportFH(BPTR fh);
BOOL Prefs_ExportFH(BPTR fh);
BOOL Prefs_Default(VOID);

/*********************************************************************************************/

extern struct AslPrefs aslprefs;

/*********************************************************************************************/

#endif /* _PREFS_H_ */
