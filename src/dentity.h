#ifndef __DENTITY__
#define __DENTITY__
#include "tools.h"

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


#endif