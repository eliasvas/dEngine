#include "dentity.h"


const unsigned DENTITY_INDEX_BITS = 22;
const unsigned DENTITY_INDEX_MASK = (1 << DENTITY_INDEX_BITS) - 1;

const unsigned DENTITY_GENERATION_BITS = 8;
const unsigned DENTITY_GENERATION_MASK = (1 << DENTITY_GENERATION_BITS) - 1;


u32 dentity_index(dEntity e) {return e.id & DENTITY_INDEX_MASK;}
u32 dentity_generation(dEntity e) {return (e.id >> DENTITY_INDEX_BITS) & DENTITY_GENERATION_MASK;}



void dentity_manager_init(dEntityManager *manager){
    if (manager == NULL)manager = &entity_manager;
    manager->generation_count = 0;
    manager->indices_start_index = 0;
    manager->indices_end_index = 0;
}

dEntity dentity_make(u32 index, u8 generation){
    return (dEntity){(generation << DENTITY_INDEX_BITS) | index};
}

b32 dentity_alive(dEntity e){
    return (entity_manager.generation[e.id] == e.id);
}

void dentity_destroy(dEntity e){
    u32 idx = e.id;
    ++entity_manager.generation[idx];
    entity_manager.free_indices[entity_manager.indices_end_index++ % MAX_FREE_INDICES] = idx;
}

dEntity dentity_create(void){
    u32 idx;

    u32 free_index_count = entity_manager.indices_end_index - entity_manager.indices_start_index;
    if (free_index_count > 500){
        idx = entity_manager.free_indices[entity_manager.indices_start_index++  % MAX_FREE_INDICES];
    }else{
        entity_manager.generation[entity_manager.generation_count++ % MAX_GENERATION] = 0;
        idx = entity_manager.generation_count - 1;
        //ensures indices dont loop through, dince generation (last 8 bits)should be 0 in ths case
        assert(idx < (1 <<  DENTITY_INDEX_BITS));
    }
    return dentity_make(idx, entity_manager.generation[idx]);
}
