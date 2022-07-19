#ifndef __DENTITY__
#define __DENTITY__
#include "tools.h"
#include "assert.h"
#include "dmem.h"
#include "dthread.h"

//this ECS pretty much follows the stingray approach
//https://bitsquid.blogspot.com/2017/05/rebuilding-entity-index.html

typedef struct dEntity
{
    u32 id;
}dEntity;
u32 dentity_index(dEntity e);
u32 dentity_generation(dEntity e);




#define MAX_FREE_INDICES 2048
#define MAX_GENERATION 2048 * 32
typedef struct dEntityManager{
    u8 generation[MAX_GENERATION];
    u32 generation_count;
    u32 free_indices[MAX_FREE_INDICES];
    u32 indices_start_index, indices_end_index;
}dEntityManager;
dEntityManager entity_manager;

void dentity_manager_init(dEntityManager *manager);
dEntity dentity_make(u32 index, u8 generation);
b32 dentity_alive(dEntity e);
void dentity_destroy(dEntity e);
dEntity dentity_create(void);




typedef struct dTransformComponentManager{
    struct InstanceData{
        u32 n; //no. of instances
        u32 allocated; //bytes allocated
        void *buffer; //where data is

        dEntity *entity;
        vec3 *translation;
        vec4 *rotation;
        vec3 *scale;
    };
    struct InstanceData data;
    u32 next_index;

    struct {u32 key; u32 value}*entity_hash;//entity ID -> array index
    dLinearAllocator data_allocator;
    dMutex m;
}dTransformComponentManager;

void dtransform_component_manager_allocate(dTransformComponentManager *manager, u32 size);

void dtransform_component_manager_init(dTransformComponentManager *manager);

u32 dtransform_component_manager_add(dTransformComponentManager *manager, dEntity e);

//Return value of 0xFFFF means entity not found
u32 dtransform_component_manager_lookup(dTransformComponentManager *manager, dEntity e);

vec3 *dtransform_component_manager_translation(dTransformComponentManager *manager, u32 index);

u32 dtransform_component_manager_simulate(dTransformComponentManager *manager);


void dtransform_component_manager_del(dTransformComponentManager *manager, u32 index);
#endif