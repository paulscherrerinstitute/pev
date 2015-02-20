#ifndef funcname_h
#define funcname_h

#ifdef __cplusplus
extern "C" {
#endif

/* Utility: return function name in allocated buffer */
/* release with free() */
char* funcName(void* funcPtr, int withFilename /* 0=no file name, 1=with file name, 2=with full path */);

#ifdef __cplusplus
}
#endif

#endif /* funcname_h */
