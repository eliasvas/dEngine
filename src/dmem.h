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

//if the chunk is free, the first 2*sizeof(void) bytes of each element are filled with the
//dPoolFreeNode, pointing to the next free node, if its filled, we just have data there, no pointer, it isn't needed!
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

//DOC: Stored at beginning of allocation to indicate size  
//HDR | PAD | PAD |---| PAD | data[0] | data[1] |---------------| data[n]
//the header contains the size of data
#define DALLOC_HEADER_PAD_VALUE 0xffffffff
struct dAllocHeader {
    u32 size;
};
inline dAllocHeader* get_header(void *data)
{
    uint32_t *p = (uint32_t *)data;
    while (p[-1] == DALLOC_HEADER_PAD_VALUE)
        --p;
    return (dAllocHeader *)p - 1;
}
inline void fill_header(dAllocHeader *header, void *data, u32 size){
    header->size = size;
    u32 *p = (u32*)(header + 1);
    while (p < data)*p++ = DALLOC_HEADER_PAD_VALUE;
}

//DOC: A memory allocator that just uses malloc. Generally not the best, opt for a custom allocator
struct dMallocAllocator {
    u32 total_allocated; // Total memory that has been allocated
    //DOC initializes the allocator
    void init(void);
    //DOC: just allocates memory, via malloc, in the alignment we want
    void *alloc(u32 size, u32 align = 0);
    //DOC: frees allocated memory
    void free(void *ptr);
    //DOC: returns the size of some allocation
    u32 alloc_size(void *ptr);
    //DOC: return the overall allocation size
    u32 alloc_total(void);
};


#endif