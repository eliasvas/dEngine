#include "dmem.h"

void *dalloc(u64 alloc_size)
{
    return malloc(alloc_size);
}

void dfree(void *alloc_src)
{
    free(alloc_src);
}


void dLinearAllocator::init(void *buf, u64 buf_len){
    this->buf = (u8*)buf;
    this->buf_len = buf_len;
    this->curr_offset = 0;
    this->prev_offset = 0;
}

void dLinearAllocator::init(u64 buf_len){
    this->buf = (u8*)dalloc(buf_len);
    this->buf_len = buf_len;
    this->curr_offset = 0;
    this->prev_offset = 0;
}

void dLinearAllocator::free(void){}

void dLinearAllocator::freeAll(void){
    this->curr_offset = 0;
    this->prev_offset = 0;
}

void *dLinearAllocator::allocAligned(u64 size, u64 align){
    u8 *curr_ptr = this->buf + this->curr_offset;
    u64 offset = align_fwd((u64)curr_ptr,align);
    offset -= (u64)this->buf;

    if (offset + size < this->buf_len){
        void *ptr = &this->buf[offset];
        this->prev_offset = offset;
        this->curr_offset = offset + size;

        memset(ptr, 0, size); //Optional
        return ptr;
    }
    return NULL;
}

void *dLinearAllocator::alloc(u64 size){
    return this->allocAligned(size, DEF_ALIGN);
}


void *dLinearAllocator::resizeAligned(void *old_memory, u64 old_size, u64 new_size, u64 align){
    assert(is_pow2(align));
    u8 *old_mem = (u8*)old_memory;

    if (old_mem == NULL || old_size == 0){
        return this->allocAligned(new_size, align);
    }else if (this->buf <= old_mem && old_mem < this->buf + this->buf_len){
        if (this->buf + this->prev_offset == old_mem){
            this->curr_offset = this->prev_offset + new_size;
            if (new_size > old_size){
                memset(&this->buf[this->curr_offset],0,new_size - old_size); //Optional
            }
            return old_memory;
        }
        else{
            void *new_memory = this->allocAligned(new_size, align);
            u64 copy_size = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }

    }
    else {
        return NULL;
    }
}

void *dLinearAllocator::resize(void *old_memory, u64 old_size, u64 new_size){
    return this->resizeAligned(old_memory, old_size, new_size, DEF_ALIGN);
}






void dPoolAllocator::init(void *buf, u32 buf_len, u32 chunk_size, u32 chunk_alignment){
    u8 *initial_start = (u8*)buf;
    u64 start = align_fwd((u64)initial_start, chunk_alignment);
    buf_len -= (start - (u64)initial_start); //because the chunks are fixed

    //align chunk size to required chunk alignment
    chunk_size = align_fwd(chunk_size, chunk_alignment); // (e.g size 15 -becomes-> 16 for chunk alignment 16 (a mat4))


    assert(chunk_size >= sizeof(dPoolFreeNode) && "Chunk size so small, \"NEXT\" pointer doesn't fit");
    assert(buf_len > chunk_size && "Chunk size bigger than buffer size");

    this->buf = (u8*)buf;
    this->buf_len = buf_len;
    this->chunk_size = chunk_size;
    this->head = NULL;

    this->freeAll();
}

void dPoolAllocator::init(u32 buf_len, u32 chunk_size, u32 chunk_alignment){
    u8 *initial_start = (u8*)dalloc(buf_len);
    u64 start = align_fwd((u64)initial_start, chunk_alignment);
    buf_len -= (start - (u64)initial_start); //because the chunks are fixed

    //align chunk size to required chunk alignment
    chunk_size = align_fwd(chunk_size, chunk_alignment); // (e.g size 15 -becomes-> 16 for chunk alignment 16 (a mat4))


    assert(chunk_size >= sizeof(dPoolFreeNode) && "Chunk size so small, \"NEXT\" pointer doesn't fit");
    assert(buf_len > chunk_size && "Chunk size bigger than buffer size");

    this->buf = (u8*)buf;
    this->buf_len = buf_len;
    this->chunk_size = chunk_size;
    this->head = NULL;

    this->freeAll();
}

void *dPoolAllocator::alloc(void){
    //get the last free node
    dPoolFreeNode *node = this->head;

    //if there is none, we have run out of space....
    if (node == NULL) {
        assert(0 && "Pool allocator has no free slots!\n");
        return NULL;
    }

    this->head = this->head->next;

    return memset(node, 0, this->chunk_size); //memset is optional
}


void dPoolAllocator::free(void *ptr){
    dPoolFreeNode *node;

    void *start = this->buf;
    void *end = &this->buf[this->buf_len];

    //if pointer to free is null, do nothing
    if (ptr == NULL)return;

    //if it is out of bounds, panic..
    if (!(start <= ptr && ptr < end)){
        assert(0 && "Pointer is out of bounds!\n");
        return;
    }

    //at the start of ptr, is the slot itself, so we make that
    //a head, and put the previous head as its 'next'
    node = (dPoolFreeNode*)ptr;
    node->next = this->head;
    this->head = node;

}

void dPoolAllocator::freeAll(void){
    u32 chunk_count = this->buf_len / this->chunk_size;

    for (u32 i = 0; i < chunk_count; ++i){
        void *ptr = &this->buf[i * this->chunk_size];
        dPoolFreeNode *node = (dPoolFreeNode*)ptr;
        //push the node to the free list
        node->next = this->head;
        this->head = node;
    }
}








void dMallocAllocator::init(void){
    memset(this, 0, sizeof(dMallocAllocator));
}

void *dMallocAllocator::alloc(u32 size, u32 align) {
    u32 ts = size + align + sizeof(dAllocHeader);
    dAllocHeader *h = (dAllocHeader*)dalloc(ts);
    void *data = (void*)align_fwd((u64)h+sizeof(h),align);
    fill_header(h, data, ts);
    this->total_allocated += ts;
    return data;
}

void dMallocAllocator::free(void *ptr){
    if (ptr == NULL)return;

    dAllocHeader *header = get_header(ptr);
    this->total_allocated -= header->size;
    dfree(header);
}

u32 dMallocAllocator::alloc_size(void *ptr) {
    return get_header(ptr)->size;
}

u32 dMallocAllocator::alloc_total(void) {
    return this->total_allocated;
}
