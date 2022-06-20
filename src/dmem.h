#ifndef DMEM_H
#define DMEM_H
#include "tools.h"

#if defined(BUILD_WIN) || defined(BUILD_UNIX)
    #include "stdlib.h"
#endif

void *dalloc(u64 alloc_size);

void dfree(void *alloc_src);

#endif