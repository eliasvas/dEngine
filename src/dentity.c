#include "dentity.h"
#include "stb/stb_ds.h"

const unsigned DENTITY_INDEX_BITS = 22;
const unsigned DENTITY_INDEX_MASK = (1 << DENTITY_INDEX_BITS) - 1;

const unsigned DENTITY_GENERATION_BITS = 8;
const unsigned DENTITY_GENERATION_MASK = (1 << DENTITY_GENERATION_BITS) - 1;

//#define DENTITY_NOT_FOUND 0xFFFFFFFF

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

dComponentField dcomponent_field_make(char *name, u32 offset, dComponentFieldType type){
    dComponentField f = {0};
    sprintf(f.name, name);
    f.type = type;
    f.offset = offset;
    return f;
}

void dcomponent_desc_insert(dComponentDesc *d, dComponentField f)
{
    dbf_push(d->fields_buf, f);
    d->field_count++; //this isn't really neaded :P
}





dTransform dtransform_default(void)
{
    dTransform t;
    t.rot = quat(0,0,0,1);
    t.trans = v3(0.0f,0.0f,0.0f);
    t.scale = v3(1.0f,1.0f,1.0f);
    return t;
}

mat4 dtransform_to_mat4(dTransform t)
{
    mat4 rotation = quat_to_mat4(t.rot);
    mat4 translation = mat4_translate(t.trans);
    mat4 scale = mat4_scale(t.scale);
    
    return mat4_mul(translation, mat4_mul(rotation, scale));
}

dTransform mat4_to_dtransform(mat4 m){
    dTransform t;
    t.rot = mat4_to_quat(m);
    t.trans = v3(m.raw[12], m.raw[13], m.raw[14]);
    t.scale = v3(m.elements[0][0], m.elements[1][1], m.elements[2][2]);
    return t;
}

//TODO: this allocate should happen after every insert if there is not enough size for new element
void dtransform_cm_allocate(dTransformCM *manager, u32 size)
{
    assert(size > manager->data.n);

    struct InstanceData new_data;
    const u32 bytes = size * (sizeof(dEntity) + sizeof(dTransform)*2 + sizeof(u32*)*5);
        
    new_data.buffer = malloc(bytes);
    new_data.n = manager->data.n;
    new_data.allocated = size;

    new_data.entity = (dEntity *)(new_data.buffer);
    new_data.local = (dTransform*)(new_data.entity + size);
    new_data.world = (dTransform*)(new_data.local + size);
    new_data.parent = (u32*)(new_data.world + size);
    new_data.first_child = (u32*)(new_data.parent + size);
    new_data.next_sibling = (u32*)(new_data.first_child + size);
    new_data.prev_sibling = (u32*)(new_data.next_sibling + size);

    if (manager->data.buffer != NULL){
        memcpy(new_data.entity, manager->data.entity, manager->data.n * sizeof(dEntity));
        memcpy(new_data.local, manager->data.local, manager->data.n * sizeof(dTransform));
        memcpy(new_data.world, manager->data.world, manager->data.n * sizeof(dTransform));
        memcpy(new_data.parent, manager->data.parent, manager->data.n * sizeof(u32*));
        memcpy(new_data.first_child, manager->data.first_child, manager->data.n * sizeof(u32*));
        memcpy(new_data.next_sibling, manager->data.next_sibling, manager->data.n * sizeof(u32*));
        memcpy(new_data.prev_sibling, manager->data.prev_sibling, manager->data.n * sizeof(u32*));

    }

    if (manager->data.buffer != NULL)
        free(manager->data.buffer);
    manager->data = new_data;
}

void dtransform_cm_init(dTransformCM *manager){
    memset(manager, 0, sizeof(dTransformCM));
    dmem_linear_init(&manager->data_allocator, malloc(10), 10);
    manager->entity_hash = NULL;
    manager->m = dmutex_create();
    dtransform_cm_allocate(manager, 100);
    hmdefault(manager->entity_hash, 0xFFFFFFFF);

    //make the component description
    dComponentField f1 = dcomponent_field_make("Translation", offsetof(dTransform, trans), DCOMPONENT_FIELD_TYPE_VEC3);
    dComponentField f2 = dcomponent_field_make("Rotation", offsetof(dTransform, rot), DCOMPONENT_FIELD_TYPE_VEC4);
    dComponentField f3 = dcomponent_field_make("Scale", offsetof(dTransform, scale), DCOMPONENT_FIELD_TYPE_VEC3);
    memset(&manager->component_desc, 0, sizeof(manager->component_desc));
    dcomponent_desc_insert(&manager->component_desc, f1);
    dcomponent_desc_insert(&manager->component_desc, f2);
    dcomponent_desc_insert(&manager->component_desc, f3);
}

u32 dtransform_cm_add(dTransformCM *manager, dEntity e, dEntity p){
    u32 component_index = manager->data.n;
    hmput(manager->entity_hash, e.id, component_index);

    manager->data.entity[component_index] = e;
    manager->data.local[component_index] = dtransform_default();
    manager->data.world[component_index] = dtransform_default();
    u32 parent_index = (p.id == DENTITY_NOT_FOUND) ? DENTITY_NOT_FOUND : hmget(manager->entity_hash, p.id);
    manager->data.parent[component_index] = parent_index;
    manager->data.first_child[component_index] = DENTITY_NOT_FOUND;
    manager->data.next_sibling[component_index] = DENTITY_NOT_FOUND;
    manager->data.prev_sibling[component_index] = DENTITY_NOT_FOUND;

    //if there is a parent, insert current component as a child
    if (parent_index != DENTITY_NOT_FOUND){
        //we find the first child of parent
        u32 child_index = manager->data.first_child[parent_index];
        if (child_index == DENTITY_NOT_FOUND){
            manager->data.first_child[parent_index] = component_index;
        }else{
            //and search for the last sibling
            while(child_index != DENTITY_NOT_FOUND)
                child_index = manager->data.next_sibling[child_index];
            //when we find it we insert our component index as the sibling (which makes it a registered child of parent!)
            manager->data.next_sibling[child_index] = component_index;
        }
       
    }



    return manager->data.n++;
}

//Return value of 0xFFFFFFFF means entity not found
u32 dtransform_cm_lookup(dTransformCM *manager, dEntity e){
    s32 component_index = hmget(manager->entity_hash, e.id);
    return component_index;
}


dTransform *dtransform_cm_local(dTransformCM *manager,u32 index){
    return &manager->data.local[index];
}

dTransform *dtransform_cm_world(dTransformCM *manager,u32 index){
    return &manager->data.world[index];
}

void dtransform_cm_del(dTransformCM *manager, u32 index){
    u32 last_component = manager->data.n-1;
    dEntity e = manager->data.entity[index];
    dEntity last_entity = manager->data.entity[last_component];

    manager->data.entity[index] = manager->data.entity[last_component];
    manager->data.world[index] = manager->data.world[last_component];
    manager->data.local[index] = manager->data.local[last_component];

    hmdel(manager->entity_hash, e.id);
    hmdel(manager->entity_hash, last_entity.id);
    hmput(manager->entity_hash, last_entity.id, index);

    manager->data.n--;
}

void dtransform_cm_set_local(dTransformCM *manager, u32 component_index, dTransform t)
{
    manager->data.local[component_index] = t;
    u32 parent_index = manager->data.parent[component_index];
    dTransform parent_trans = dtransform_default();
    if (parent_index != DENTITY_NOT_FOUND)
        parent_trans = manager->data.world[parent_index];
    dtransform_cm_transform(manager,parent_trans, component_index);
}

void dtransform_cm_transform(dTransformCM *manager, dTransform  parent, u32 component_index)
{
    mat4 world = mat4_mul(dtransform_to_mat4(manager->data.local[component_index]), dtransform_to_mat4(parent));
    manager->data.world[component_index] = mat4_to_dtransform(world);

    u32 child_index = manager->data.first_child[component_index];
    while (child_index != DENTITY_NOT_FOUND) {
       dtransform_cm_transform(manager, manager->data.world[component_index], child_index);
       child_index = manager->data.next_sibling[child_index];
    }
}



u32 dtransform_cm_simulate(dTransformCM *manager){

}