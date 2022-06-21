#ifndef DMEM_H
#define DMEM_H
#include "tools.h"

#if defined(BUILD_WIN) || defined(BUILD_UNIX)
    #include "stdlib.h"
#endif

void *dalloc(u64 alloc_size);

void dfree(void *alloc_src);

typedef struct 
{
    u8 *buf;
    u64 buf_len;
    u64 prev_offset;
    u64 curr_offset;
}dLinearAllocator;
void dmem_linear_init(dLinearAllocator *a, void *buf, u64 buf_len);
void dmem_linear_free(dLinearAllocator *a);
void dmem_linear_free_all(dLinearAllocator *a);
void *dmem_linear_alloc(dLinearAllocator *a, u64 size);
void *dmem_linear_resize(dLinearAllocator *a, void *old_memory, u64 old_size, u64 new_size);

#endif