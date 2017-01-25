#ifndef PLUGIN_H_
#define PLUGIN_H_

#include <windows.h>

// TODO use __stdcall
#define EXPORT_CALLING __cdecl
#define EXPORT __declspec(dllexport) EXPORT_CALLING

// just defined to make compilation work ; ignored
#define RTLD_LOCAL   -1
#define RTLD_LAZY    -1
#define RTLD_NOW     -1

static inline void *dlopen(const char *name, int flags)
{
    (void)flags;
    // TODO use LoadLibraryEx() and flags ?
    return LoadLibrary(name);
}

static inline void *dlsym(void *handle, const char *name)
{
    FARPROC g = GetProcAddress(handle, name);
    void *h;
    *(FARPROC*)&h = g;
    return h;
}

static inline char *dlerror(void)
{
    static __thread char buf[1024];

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&buf,
        sizeof buf, NULL);

    return buf;
}

static inline int dlclose(void *handle)
{
    return !FreeLibrary(handle);
}

#include "plugin_portable.h"

#endif

/* vi: set ts=4 sw=4 et: */
