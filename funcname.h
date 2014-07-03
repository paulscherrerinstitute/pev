#ifndef funcname_h
#define funcname_h

#ifdef __cplusplus
extern "C" {
#endif

/* Utility: return function name in allocated buffer */
/* release with free() */
char* funcName(void* funcPtr, int withFilename);

#ifdef __cplusplus
}
#endif

#endif /* funcname_h */
