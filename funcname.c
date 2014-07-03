/* Utility: return function name in allocated buffer */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <c++/cxxabi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "funcname.h"

char* funcName(void* funcPtr, int withFilename)
{
    Dl_info sym;
    size_t l;
    char *name;
    char *fname;
    int demangle_failed;

    if (!funcPtr) return strdup("NULL");
    memset(&sym, 0, sizeof(sym));
    dladdr(funcPtr, &sym);
    if (!sym.dli_fname) withFilename = 0;
    fname = __cxa_demangle(sym.dli_sname, NULL, &l, &demangle_failed);
    if (demangle_failed)
    {
        fname = (char*)sym.dli_sname;
        l = fname ? strlen(fname) : 0;
    }        
    name = malloc(l + 20 + (withFilename ? strlen(sym.dli_fname) + 3 : 0));
    if (name)
    {
        if (l) strcpy(name, fname);
        if (funcPtr != sym.dli_saddr)
            l += sprintf(name + l, "%s0x%x", l ? "+" : "", funcPtr - sym.dli_saddr);
        if (withFilename)
            sprintf(name + l, " (%s)", sym.dli_fname);
    }
    if (!demangle_failed) free(fname);
    return name;
}
