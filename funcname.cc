/* Utility: return function name in allocated buffer */
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cxxabi.h>
#include "funcname.h"

extern "C"
char* funcName(void* funcPtr, int withFilename /* 1=file, 2=full path */)
{
    Dl_info sym;
    size_t l;
    char *name;
    char *funcName;
    const char *fileName;
    int demangleFailed;

    if (!funcPtr) return strdup("NULL");
    memset(&sym, 0, sizeof(sym));
    dladdr(funcPtr, &sym);
    if (!sym.dli_fname) withFilename = 0;
    funcName = __cxxabiv1::__cxa_demangle(sym.dli_sname, NULL, &l, &demangleFailed);
    if (demangleFailed)
    {
        funcName = (char*)sym.dli_sname;
        l = funcName ? strlen(funcName) : 0;
    }
    fileName = sym.dli_fname;
    if (withFilename == 1)
    {
        const char *p = strrchr(fileName, '/');
        if (p) fileName = p+1;
    }        
    name = (char*) malloc(l + 20 + (withFilename ? strlen(fileName) + 3 : 0));
    if (name)
    {
        if (l) strcpy(name, funcName);
        if (funcPtr != sym.dli_saddr)
            l += sprintf(name + l, "%s0x%x", l ? "+" : "", (char*)funcPtr - (char*)sym.dli_saddr);
        if (withFilename)
            sprintf(name + l, " (%s)", fileName);
    }
    if (!demangleFailed) free(funcName);
    return name;
}
