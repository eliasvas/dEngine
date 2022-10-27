#include "dhandle.h"

//The handle system is based on this:
//https://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html


const unsigned DHANDLE_INDEX_BITS = 22;
const unsigned DHANDLE_INDEX_MASK = (1 << DHANDLE_INDEX_BITS) - 1;

const unsigned DHANDLE_GENERATION_BITS = 8;
const unsigned DHANDLE_GENERATION_MASK = (1 << DHANDLE_GENERATION_BITS) - 1;



u32 dHandle::index(void) {return id & DHANDLE_INDEX_MASK;}
u32 dHandle::generation(void) {return (id >> DHANDLE_INDEX_BITS) & DHANDLE_GENERATION_MASK;}


dHandle dhandle_make(u32 index, u8 generation){
    return (dHandle){(generation << DHANDLE_INDEX_BITS) | index};
}

void dHandleManager::init(void){
    this->free_indices.init(16);
    this->generation.init(16);
}

void dHandleManager::deinit(void){
    this->free_indices.deinit();
    this->generation.deinit();
}

b32 dHandleManager::alive(dHandle h){
    assert(generation.size() >= h.index());
    return (generation[h.index()] == h.generation());
}

dHandle dHandleManager::create(void){
    u32 index;

    if(free_indices.size() > 512){
        index = free_indices[0];
        free_indices.pop_front();
    }else{
        index = generation.size();
        generation.push_back(0);
        assert(index < (1 << DHANDLE_INDEX_BITS)); //we can't have more than DHANDLE_INDEX_BITS handles together
    }
    return dhandle_make(index, generation[index]);
}
