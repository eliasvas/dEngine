#include "dparticle.h"
#include "dgfx.h"
#include "deditor.h"
#include "dmem.h"
#include "stb/stb_ds.h"

extern dTransformCM transform_manager;

dParticleEmitter test_emitter;


//Initializes the buffers needed to render and update particles
void dparticle_system_init(void){
    dparticle_emitter_init(&test_emitter);
}


//if TRUE particle is fine, if FALSE simulation has finished
b32 dparticle_update(dParticle *p, f32 dt) {
    p->vel.y -= DPARTICLE_GRAVITY * p->grav_effect * dt;
    vec3 change = p->vel;
    change = vec3_mulf(change, dt);
    p->pos = vec3_add(p->pos, change);
    p->elapsed_time +=dt;
    return (p->elapsed_time < p->life_length);
}

extern mat4 view;
extern dgDevice dd;
extern dEditor main_editor;
extern dgRT def_rt;
extern dgBuffer base_vbo;
extern dgBuffer base_pos;
extern dgBuffer base_tex;
extern dgBuffer base_ibo;

void dparticle_render(dParticle *p, vec4 col1, vec4 col2){
    dgDevice *ddev = &dd;
    //dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev, main_editor.viewport.x,main_editor.viewport.y,main_editor.viewport.z-main_editor.viewport.x, main_editor.viewport.w -main_editor.viewport.y);
    dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_bind_pipeline(ddev, &ddev->particle_pipe);
    mat4 model= mat4_translate(p->pos);
    dgBuffer buffers[] = {base_pos, base_tex};
    u64 offsets[] = {0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);
    
    f32 progress = (p->life_length - p->elapsed_time)/ p->life_length;
    vec4 col= v4(lerp(col1.x, col2.x, progress),lerp(col1.y, col2.y, progress),lerp(col1.z, col2.z, progress),lerp(col1.w, col2.w, progress));
    //mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
    //mat4 object_data = mat4_mul(mat4_translate(v3(0,1 * fabs(sin(5 * dtime_sec(dtime_now()))),-15)), mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7)));
    mat4 object_data[2] = {model, {col.x, col.y, col.z, col.w}};
    dg_set_desc_set(ddev,&ddev->particle_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 4,6);

    //dg_rendering_end(ddev);
}

void dparticle_emitter_init(dParticleEmitter *emitter){
    emitter->particle_count = 0;
    emitter->pps = 70;
    emitter->color1 = v4(0.0,0.0,0.0,1.0);
    emitter->color2 = v4(1.0,1.0,1.0,1.0);
    emitter->local_position = v3(0,0,0);
}

extern mat4 view, proj;
void dparticle_emitter_render(dParticleEmitter *emitter){
    //set desc set 0
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(&dd,&dd.particle_pipe, data, sizeof(data), 0);
    dg_rendering_begin(&dd, NULL, 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    for (u32 i =0; i < emitter->particle_count;++i){
        dparticle_render(&emitter->p[i], emitter->color1, emitter->color2);
    }
    dg_rendering_end(&dd);
}

void dparticle_emitter_del(dParticleEmitter *emitter, u32 index){
    if (emitter->particle_count)
        emitter->p[index] = emitter->p[emitter->particle_count--];
}

void dparticle_emitter_add(dParticleEmitter *emitter, dParticle p){
    emitter->p[emitter->particle_count++] = p;
}

void dparticle_emitter_emit(dParticleEmitter *emitter){
    dParticle p = {0};
    float dir_x = r01() * 2.0f - 1.0f;
    float dir_y = r01() * 2.0f - 1.0f;
    vec3 vel = v3(dir_x, 1, dir_y);
    vel = vec3_normalize(vel);
    p.vel = vel;
    p.pos = emitter->world_position;
    p.grav_effect = 1;
    p.scale =1.0f;
    p.life_length= 1.0;
    if (emitter->particle_count < DPARTICLE_EMITTER_MAX_PARTICLES)
        dparticle_emitter_add(emitter, p);
}

void dparticle_emitter_gen_particles(dParticleEmitter *emitter, f32 dt){
    u32 particles_to_create = (f32)(dt * emitter->pps);
    for (u32 i = 0; i < particles_to_create; ++i){
        dparticle_emitter_emit(emitter);
    }
}

void dparticle_emitter_update(dParticleEmitter *emitter, f32 dt){
    //printf("particle count: %d\n", emitter->particle_count);
    dparticle_emitter_gen_particles(emitter, dt);
    for (u32 i =0; i < emitter->particle_count;++i){
        if (!dparticle_update(&emitter->p[i], dt))
            dparticle_emitter_del(emitter, i);
    }
}


dParticleEmitterCM particle_emitter_cm;

//TODO: this allocate should happen after every insert if there is not enough size for new element
void dparticle_emitter_cm_allocate(dParticleEmitterCM *manager, u32 size)
{
    if (manager == NULL)manager = &particle_emitter_cm;
    assert(size > manager->data.n);

    struct peInstanceData new_data;
    const u32 bytes = size * (sizeof(dEntity) + sizeof(dParticleEmitter));
        
    new_data.buffer = dalloc(bytes);
    new_data.n = manager->data.n;
    new_data.allocated = size;

    new_data.entity = (dEntity *)(new_data.buffer);
    new_data.emitter = (dTransform*)(new_data.entity + size);

    if (manager->data.buffer != NULL){ //if we have data from previous allocation, copy.
        memcpy(new_data.entity, manager->data.entity, manager->data.n * sizeof(dEntity));
        memcpy(new_data.emitter, manager->data.emitter, manager->data.n * sizeof(dParticleEmitter));
    }

    if (manager->data.buffer != NULL)
        dfree(manager->data.buffer);
    manager->data = new_data;
}

void dparticle_emitter_cm_init(dParticleEmitterCM *manager){
    if (manager == NULL)manager = &particle_emitter_cm;

    memset(manager, 0, sizeof(dParticleEmitterCM));
    manager->entity_hash = NULL;
    dparticle_emitter_cm_allocate(manager, 10);
    hmdefault(manager->entity_hash, 0xFFFFFFFF);
    dComponentField f1 = dcomponent_field_make("world_position", offsetof(dParticleEmitter, world_position), DCOMPONENT_FIELD_TYPE_VEC3);
    dComponentField f3 = dcomponent_field_make("local_position", offsetof(dParticleEmitter, local_position), DCOMPONENT_FIELD_TYPE_VEC3);
    dComponentField f2 = dcomponent_field_make("pps", offsetof(dParticleEmitter, pps), DCOMPONENT_FIELD_TYPE_U32);
    memset(&manager->component_desc, 0, sizeof(manager->component_desc));
    dcomponent_desc_insert(&manager->component_desc, f1);
    dcomponent_desc_insert(&manager->component_desc, f2);
    dcomponent_desc_insert(&manager->component_desc, f3);
}


u32 dparticle_emitter_cm_add(dParticleEmitterCM *manager, dEntity e){
    if (manager == NULL)manager = &particle_emitter_cm;

    //Find the next index available by the manager and create an EntityID -> index relation
    u32 component_index = manager->data.n;
    hmput(manager->entity_hash, e.id, component_index);

    //Initialize the component's data in that index
    manager->data.entity[component_index] = e;
    dparticle_emitter_init(&manager->data.emitter[component_index]);

    //return the index and increment the global index of the manager, to be ready for the next insertion
    return manager->data.n++;
}


//Return value of 0xFFFFFFFF means entity not found
u32 dparticle_emitter_cm_lookup(dParticleEmitterCM *manager, dEntity e){
    if (manager == NULL)manager = &particle_emitter_cm;
    s32 component_index = hmget(manager->entity_hash, e.id);
    return component_index;
}

void dparticle_emitter_cm_del(dParticleEmitterCM *manager, u32 index){
    if (manager == NULL)manager = &particle_emitter_cm;
    u32 last_component = manager->data.n-1;
    dEntity e = manager->data.entity[index];
    dEntity last_entity = manager->data.entity[last_component];

    manager->data.entity[index] = manager->data.entity[last_component];
    manager->data.emitter[index] = manager->data.emitter[last_component];

    hmdel(manager->entity_hash, e.id);
    hmdel(manager->entity_hash, last_entity.id);
    hmput(manager->entity_hash, last_entity.id, index);

    manager->data.n--;
}

//we update all the emitters
u32 dparticle_emitter_cm_simulate(dParticleEmitterCM *manager, f32 dt){
    if (manager == NULL)manager = &particle_emitter_cm;

    
    for (u32 i = 0; i < manager->data.n; ++i){
        dparticle_emitter_update(&manager->data.emitter[i], dt);
    }
    return TRUE;
}

//we render all the emitters
u32 dparticle_emitter_cm_render(dParticleEmitterCM *manager){
    if (manager == NULL)manager = &particle_emitter_cm;


    


    for (u32 i = 0; i < manager->data.n; ++i){

        //if there is a transform component for our entity, add that to the position of the emitter
        dEntity current_entity = manager->data.entity[i];
        u32 transform_index = dtransform_cm_lookup(&transform_manager, current_entity);
        vec3 transform_pos= v3(0,0,0);
        if(transform_index != DENTITY_NOT_FOUND){
            dTransform lp = transform_manager.data.world[transform_index]; 
            transform_pos = lp.trans;
        }
        manager->data.emitter[i].world_position = vec3_add(manager->data.emitter[i].local_position, transform_pos);
        dparticle_emitter_render(&manager->data.emitter[i]);
    }
    return TRUE;
}

dParticleEmitter *dparticle_emitter_cm_emitter(dParticleEmitterCM *manager,u32 index){
    if (manager == NULL)manager = &particle_emitter_cm;

    return &manager->data.emitter[index];
}