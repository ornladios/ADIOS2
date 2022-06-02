#include <dlfcn.h>
#define lt_dlopen(x) CMdlopen(cm, x, 0)
#define lt_dladdsearchdir(x) CMdladdsearchdir(x)
#define lt_dlsym(x, y) CMdlsym(x, y)
#define lt_dlhandle void*
#define MODULE_EXT CMAKE_SHARED_MODULE_SUFFIX
extern void CMdladdsearchdir(char *dir);
extern void* CMdlopen(void *CMTrace_file, char *library, int mode);
extern void CMdlclose(void *handle);
extern void CMdlclearsearchlist();
extern void* CMdlsym(void *handle, char *symbol);
extern void CMset_dlopen_verbose(int verbose);
