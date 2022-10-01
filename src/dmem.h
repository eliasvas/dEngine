#ifndef DMEM_H
#define DMEM_H
#include "tools.h"

//[REF] Memory alignment basics: https://developer.ibm.com/articles/pa-dalign/
//[REF] x86 tidbits: www.rcollins.org/articles/pmbasics/tspec_a1_doc.html
//[REF] A lot of allocator implementations: https://www.gingerbill.org/series/memory-allocation-strategies/
//[REF] Cool interface: https://github.com/niklas-ourmachinery/bitsquid-foundation/blob/master/memory.h 

#if defined(BUILD_WIN) || defined(BUILD_UNIX)
    #include "stdlib.h"
#endif

void *dalloc(u64 alloc_size);

void dfree(void *alloc_src);

const u32 DEF_ALIGN = 2 * sizeof(void*);

struct dLinearAllocator {

    
    
    u8 *buf;
    u64 buf_len;
    u64 prev_offset;
    u64 curr_offset;
    void init(void *buf, u64 buf_len);
    void init(u64 buf_len);
    void free(void);
    void freeAll(void);
    void *alloc(u64 size);
    void *allocAligned(u64 size, u64 align); //private?

    void *resize(void * old_memory, u64 old_size, u64 new_size);
    void *resizeAligned(void *old_memory, u64 old_size, u64 new_size, u64 align);
};

struct dPoolFreeNode{
    dPoolFreeNode *next;
};
struct dPoolAllocator {
    u8 *buf;
    u32 buf_len;
    u32 chunk_size;

    //head of linked list containing next free chunk, for allocation
    dPoolFreeNode *head;

    void init(void *buf, u32 buf_len, u32 chunk_size, u32 chunk_alignment);
    void init(u32 buf_len, u32 chunk_size, u32 chunk_alignment);
    void *alloc(void);
    void free(void *ptr);
    void freeAll(void);
};

#endif