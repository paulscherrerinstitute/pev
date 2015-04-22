/* Utility: return function name in allocated buffer */
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cxxabi.h>
#include "symbolname.h"

extern "C"
char* symbolName(void* ptr, int withFilename /* 1=file, 2=full path */)
{
    Dl_info sym;
    size_t funcNameLength;
    size_t totalLength;
    char *result;
    char *funcName;
    const char *fileName;
    int demangleFailed;

    if (!ptr) return strdup("NULL");
    memset(&sym, 0, sizeof(sym));
    dladdr(ptr, &sym);
    if (!sym.dli_fname) withFilename = 0;
    funcName = __cxxabiv1::__cxa_demangle(sym.dli_sname, NULL, NULL, &demangleFailed);
    if (demangleFailed)
        funcName = (char*)sym.dli_sname;
    funcNameLength = funcName ? strlen(funcName) : 0;
    totalLength = funcNameLength;
    fileName = sym.dli_fname;
    if (withFilename > 0)
    {
        if (withFilename == 1)
        {
            const char *p = strrchr(fileName, '/');
            if (p) fileName = p+1;
        }
        totalLength += strlen(fileName) + 3;
    }
    if (ptr != sym.dli_saddr)
        totalLength += 20;
    
    result = (char*) malloc(totalLength);
    if (result != NULL)
    {
        if (funcNameLength != 0) strcpy(result, funcName);
        if (ptr != sym.dli_saddr)
            funcNameLength += sprintf(result + funcNameLength,
                "%s0x%x", funcNameLength ? "+" : "", (char*)ptr - (char*)sym.dli_saddr);
        if (withFilename > 0)
            sprintf(result + funcNameLength, " (%s)", fileName);
    }
    if (!demangleFailed) free(funcName);
    return result;
}
