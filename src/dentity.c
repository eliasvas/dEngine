#include "dentity.h"
#include "stb/stb_ds.h"

const unsigned DENTITY_INDEX_BITS = 22;
const unsigned DENTITY_INDEX_MASK = (1 << DENTITY_INDEX_BITS) - 1;

const unsigned DENTITY_GENERATION_BITS = 8;
const unsigned DENTITY_GENERATION_MASK = (1 << DENTITY_GENERATION_BITS) - 1;

#define DENTITY_NOT_FOUND 0xFFFFFFFF

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


void dtransform_component_manager_allocate(dTransformComponentManager *manager, u32 size)
{
    assert(size > manager->data.n);

    struct InstanceData new_data;
    const u32 bytes = size * (sizeof(dEntity) + sizeof(vec4) +
        2 * sizeof(vec3));
        
    new_data.buffer = malloc(bytes);
    new_data.n = manager->data.n;
    new_data.allocated = size;

    new_data.entity = (dEntity *)(new_data.buffer);
    new_data.translation = (vec3*)(new_data.entity + size);
    new_data.rotation = (vec4*)(new_data.translation + size);
    new_data.scale = new_data.rotation + size;
    
    if (manager->data.buffer != NULL){
        memcpy(new_data.entity, manager->data.entity, manager->data.n * sizeof(dEntity));
        memcpy(new_data.translation, manager->data.translation, manager->data.n * sizeof(vec3));
        memcpy(new_data.rotation, manager->data.rotation, manager->data.n * sizeof(vec4));
        memcpy(new_data.scale, manager->data.scale, manager->data.n * sizeof(vec3));
    }

    if (manager->data.buffer != NULL)
        free(manager->data.buffer);
    manager->data = new_data;
}

void dtransform_component_manager_init(dTransformComponentManager *manager){
    memset(manager, 0, sizeof(dTransformComponentManager));
    dmem_linear_init(&manager->data_allocator, malloc(10), 10);
    manager->entity_hash = NULL;
    manager->m = dmutex_create();
    dtransform_component_manager_allocate(manager, 100);
    hmdefault(manager->entity_hash, 0xFFFFFFFF);

}

u32 dtransform_component_manager_add(dTransformComponentManager *manager, dEntity e){
    hmput(manager->entity_hash, e.id, manager->data.n);
    manager->data.entity[manager->data.n] = e;
    return manager->data.n++;
}

//Return value of -1 means entity not found
u32 dtransform_component_manager_lookup(dTransformComponentManager *manager, dEntity e){
    s32 component_index = hmget(manager->entity_hash, e.id);
    return component_index;
}

vec3 *dtransform_component_manager_translation(dTransformComponentManager *manager, u32 index){
    return &manager->data.translation[index];
}


void dtransform_component_manager_del(dTransformComponentManager *manager, u32 index){
    u32 last_component = manager->data.n-1;
    dEntity e = manager->data.entity[index];
    dEntity last_entity = manager->data.entity[last_component];

    manager->data.entity[index] = manager->data.entity[last_component];
    manager->data.translation[index] = manager->data.translation[last_component];
    manager->data.rotation[index] = manager->data.rotation[last_component];
    manager->data.rotation[index] = manager->data.rotation[last_component];

    hmdel(manager->entity_hash, e.id);
    hmdel(manager->entity_hash, last_entity.id);
    hmput(manager->entity_hash, last_entity.id, index);

    manager->data.n--;
}



u32 dtransform_component_manager_simulate(dTransformComponentManager *manager){

}