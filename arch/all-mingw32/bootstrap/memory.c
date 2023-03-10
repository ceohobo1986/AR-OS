/*
    Copyright (C) 1995-2022, The AROS Development Team. All rights reserved.
*/

#include <io.h>
#include <stdio.h>
#include <windows.h>

#include "sharedmem.h"

/* This is not included in my Mingw32 headers */
#ifndef FILE_MAP_EXECUTE
#define FILE_MAP_EXECUTE 0x0020
#endif

#define D(x)
#define ID_LEN 64

HANDLE RAM_Handle = NULL;
void *RAM_Address = NULL;
#if defined(__x86_64__)
HANDLE RAM64_Handle = NULL;
void *RAM64_Address = NULL;
#endif

void *AllocateRO(size_t len)
{
    return VirtualAlloc(NULL, len, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

/*
 * Commit executable and read-only state for kickstart's .code
 */
int SetRO(void *addr, size_t len)
{
    DWORD old;

    return !VirtualProtect(addr, len, PAGE_EXECUTE_READ, &old);
}

void *AllocateRW(size_t len)
{
    return VirtualAlloc(NULL, len, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

void *AllocateRAM32(size_t len)
{
    SECURITY_ATTRIBUTES sa;
    void *addr = NULL;
    const char *var = getenv(SHARED_RAM_VAR);

    if (var)
    {
        D(fprintf(stderr, "[AllocateRAM32] Found RAM specification: %s\n", var));
        if (sscanf(var, "%p:%p", &RAM_Handle, &addr) != 2) {
            D(fprintf(stderr, "[AllocateRAM32] Error parsing specification\n"));
            RAM_Handle = NULL;
            addr = NULL;
        }
    }
    D(fprintf(stderr, "[AllocateRAM32] Inherited memory handle 0x%p address 0x%p\n", RAM_Handle, addr));

    if (!RAM_Handle)
    {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        RAM_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_EXECUTE_READWRITE, 0, len, NULL);
    }
    if (!RAM_Handle)
    {
        D(fprintf(stderr, "[AllocateRAM32] PAGE_EXECUTE_READWRITE failed, retrying with PAGE_READWRITE\n"));
        RAM_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, len, NULL);
    }
    D(fprintf(stderr, "[AllocateRAM32] Shared memory handle 0x%p\n", RAM_Handle));
    if (!RAM_Handle)
        return NULL;

    RAM_Address = MapViewOfFileEx(RAM_Handle, FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE, 0, 0, 0, addr);
    if (!RAM_Address)
    {
        D(fprintf(stderr, "[AllocateRAM32] FILE_MAP_EXECUTE failed, retrying without it\n"));
        RAM_Address = MapViewOfFileEx(RAM_Handle, FILE_MAP_ALL_ACCESS, 0, 0, 0, addr);
    }
    D(fprintf(stderr, "[AllocateRAM32] Mapped at 0x%p\n", RAM_Address));

    if (!RAM_Address)
    {
        CloseHandle(RAM_Handle);
        RAM_Handle = NULL;
    }

    return RAM_Address;
}

#if defined(__x86_64__)
void *AllocateRAM(size_t len)
{
    SECURITY_ATTRIBUTES sa;
    void *addr = NULL;
    const char *var = getenv(SHARED_RAM_VAR);

    if (var)
    {
        D(fprintf(stderr, "[AllocateRAM] Found RAM specification: %s\n", var));
        if (sscanf(var, "%p:%p", &RAM64_Handle, &addr) != 2) {
            D(fprintf(stderr, "[AllocateRAM] Error parsing specification\n"));
            RAM64_Handle = NULL;
            addr = NULL;
        }
    }
    D(fprintf(stderr, "[AllocateRAM] Inherited memory handle 0x%p address 0x%p\n", RAM64_Handle, addr));

    if (!RAM64_Handle)
    {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        RAM64_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_EXECUTE_READWRITE, 0, len, NULL);
    }
    if (!RAM64_Handle)
    {
        D(fprintf(stderr, "[AllocateRAM] PAGE_EXECUTE_READWRITE failed, retrying with PAGE_READWRITE\n"));
        RAM64_Handle = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, len, NULL);
    }
    D(fprintf(stderr, "[AllocateRAM] Shared memory handle 0x%p\n", RAM64_Handle));
    if (!RAM64_Handle)
        return NULL;

    RAM64_Address = MapViewOfFileEx(RAM64_Handle, FILE_MAP_ALL_ACCESS|FILE_MAP_EXECUTE, 0, 0, 0, addr);
    if (!RAM64_Address)
    {
        D(fprintf(stderr, "[AllocateRAM] FILE_MAP_EXECUTE failed, retrying without it\n"));
        RAM64_Address = MapViewOfFileEx(RAM64_Handle, FILE_MAP_ALL_ACCESS, 0, 0, 0, addr);
    }
    D(fprintf(stderr, "[AllocateRAM] Mapped at 0x%p\n", RAM64_Address));

    if (!RAM64_Address)
    {
        CloseHandle(RAM64_Handle);
        RAM64_Handle = NULL;
    }

    return RAM64_Address;
}
#endif
