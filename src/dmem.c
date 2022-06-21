#include "dmem.h"

void *dalloc(u64 alloc_size)
{
    return malloc(alloc_size);
}

void dfree(void *alloc_src)
{
    free(alloc_src);
}


void dmem_linear_init(dLinearAllocator *a, void *buf, u64 buf_len){
    a->buf = buf;
    a->buf_len = buf_len;
    a->curr_offset = 0;
    a->prev_offset = 0;
}

void dmem_linear_free(dLinearAllocator *a){}

void dmem_linear_free_all(dLinearAllocator *a){
    a->curr_offset = 0;
    a->prev_offset = 0;
}

void *dmem_linear_alloc_align(dLinearAllocator *a, u64 size, u64 align){
    u8 *curr_ptr = a->buf + a->curr_offset;
    u64 offset = align_fwd((u64)curr_ptr,align);
    offset -= (u64)a->buf;

    if (offset + size < a->buf_len){
        void *ptr = &a->buf[offset];
        a->prev_offset = offset;
        a->curr_offset = offset + size;

        memset(ptr, 0, size);
        return ptr;
    }
    return NULL;
}

#if !defined(DEF_ALIGNMENT)
    #define DEF_ALIGNMENT (2 *sizeof(void*))
#endif

void *dmem_linear_alloc(dLinearAllocator *a, u64 size){
    return dmem_linear_alloc_align(a, size, DEF_ALIGNMENT);
}


void *dmem_linear_resize_align(dLinearAllocator *a, void *old_memory, u64 old_size, u64 new_size, u64 align){
    assert(is_pow2(align));
    u8 *old_mem = old_memory;

    if (old_mem == NULL || old_size == 0){
        return dmem_linear_alloc_align(a, new_size, align);
    }else if (a->buf <= old_mem && old_mem < a->buf + a->buf_len){
        if (a->buf + a->prev_offset == old_mem){
            a->curr_offset = a->prev_offset + new_size;
            if (new_size > old_size){
                memset(&a->buf[a->curr_offset],0,new_size - old_size);
            }
            return old_memory;
        }
        else{
            void *new_memory = dmem_linear_alloc_align(a, new_size, align);
            u64 copy_size = old_size < new_size ? old_size : new_size;
            memmove(new_memory, old_memory, copy_size);
            return new_memory;
        }

    }
    else {
        return NULL;
    }
}

void *dmem_linear_resize(dLinearAllocator *a, void *old_memory, u64 old_size, u64 new_size){
    return dmem_linear_resize_align(a, old_memory, old_size, new_size, DEF_ALIGNMENT);
}