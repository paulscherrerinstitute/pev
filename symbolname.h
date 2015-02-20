#ifndef symbolname_h
#define symbolname_h

#ifdef __cplusplus
extern "C" {
#endif

/* Utility: resolve pointer to function or variable name in allocated buffer */
/* release with free() */
char* symbolName(void* ptr, int withFilename /* 0=no file name, 1=with file name, 2=with full path */);

#ifdef __cplusplus
}
#endif

#endif /* symbolname_h */
