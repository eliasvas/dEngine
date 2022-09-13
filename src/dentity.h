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

void dentity_manager_init(dEntityManager *manager);
dEntity dentity_make(u32 index, u8 generation);
b32 dentity_alive(dEntity e);
void dentity_destroy(dEntity e);
dEntity dentity_create(void);
#define DENTITY_NOT_FOUND 0xFFFFFFFF

//these define a component's editable properties, which are used by the UI system!
typedef enum dComponentFieldType{
    DCOMPONENT_FIELD_TYPE_NONE = 0x0,
    DCOMPONENT_FIELD_TYPE_F32 = 0x1,
    DCOMPONENT_FIELD_TYPE_VEC2 = 0x2,
    DCOMPONENT_FIELD_TYPE_VEC3 = 0x4,
    DCOMPONENT_FIELD_TYPE_VEC4 = 0x8,
    DCOMPONENT_FIELD_TYPE_MAT4 = 0x10,
    DCOMPONENT_FIELD_TYPE_U32 = 0x20,
}dComponentFieldType;

typedef struct dComponentField{
    char name[32];
    u32 offset; //offset in struct of Component (so we can modify it)
    dComponentFieldType type;
}dComponentField;

//NOTE: dComponentDesc must be zero initialized instead of init'ed, its the same process
typedef struct dComponentDesc{
    u32 field_count;
    dComponentField *fields_buf;
}dComponentDesc;


typedef struct dTransform{
    vec3 trans;
    Quaternion rot;
    vec3 scale;
}dTransform;

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
    dComponentDesc component_desc;

    struct {u32 key; u32 value;}*entity_hash;//entity ID -> array index
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


dComponentField dcomponent_field_make(char *name, u32 offset, dComponentFieldType type);
void dcomponent_desc_insert(dComponentDesc *d, dComponentField f);
#endif