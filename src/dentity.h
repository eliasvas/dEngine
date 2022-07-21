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


typedef struct dTransform{
    vec3 trans;
    Quaternion rot;
    vec3 scale;
}dTransform;


#define DENTITY_NOT_FOUND 0xFFFFFFFF

dTransform dtransform_default(void);
mat4 dtransform_to_mat4(dTransform t);
dTransform mat4_to_dtransform(mat4 m);

typedef struct dTransformCM{
    struct InstanceData{
        u32 n; //no. of instances
        u32 allocated; //bytes allocated
        void *buffer; //where data is

        dEntity *entity;
        dTransform *local;
        dTransform *world;

        u32 *parent;
        u32 *first_child;
        u32 *next_sibling;
        u32 *prev_sibling;
    };
    struct InstanceData data;
    u32 next_index;

    struct {u32 key; u32 value}*entity_hash;//entity ID -> array index
    dLinearAllocator data_allocator;
    dMutex m;
}dTransformCM;

void dtransform_cm_allocate(dTransformCM *manager, u32 size);

void dtransform_cm_init(dTransformCM *manager);

u32 dtransform_cm_add(dTransformCM *manager, dEntity e, dEntity p);

//Return value of 0xFFFF means entity not found
u32 dtransform_cm_lookup(dTransformCM *manager, dEntity e);

dTransform *dtransform_cm_local(dTransformCM *manager,u32 index);
dTransform *dtransform_cm_world(dTransformCM *manager,u32 index);

u32 dtransform_cm_simulate(dTransformCM *manager);

void dtransform_cm_set_local(dTransformCM *manager, u32 component_index, dTransform t);

void dtransform_cm_del(dTransformCM *manager, u32 index);
#endif