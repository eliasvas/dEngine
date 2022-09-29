#include "dparticle.h"
#include "dgfx.h"
#include "deditor.h"
#include "dmem.h"
#include "stb/stb_ds.h"

extern dTransformCM transform_manager;


//if TRUE particle is fine, if FALSE simulation has finished
b32 dParticle::update(f32 dt) {
    this->vel.y -= DPARTICLE_GRAVITY * this->grav_effect * dt;
    vec3 change = this->vel;
    change = vec3_mulf(change, dt);
    this->pos = vec3_add(this->pos, change);
    this->elapsed_time +=dt;
    return (this->elapsed_time < this->life_length);
}

extern mat4 view;
extern dgDevice dd;
extern dEditor main_editor;
extern dgRT def_rt;
extern dgBuffer base_vbo;
extern dgBuffer base_pos;
extern dgBuffer base_tex;
extern dgBuffer base_ibo;

void dParticle::render(vec4 col1, vec4 col2){
    dgDevice *ddev = &dd;
    //dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev, main_editor.viewport.x,main_editor.viewport.y,main_editor.viewport.z-main_editor.viewport.x, main_editor.viewport.w -main_editor.viewport.y);
    dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_bind_pipeline(ddev, &ddev->particle_pipe);
    mat4 model= mat4_translate(this->pos);
    dgBuffer buffers[] = {base_pos, base_tex};
    u64 offsets[] = {0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);
    
    f32 progress = (this->life_length - this->elapsed_time)/ this->life_length;
    vec4 col= v4(lerp(col1.x, col2.x, progress),lerp(col1.y, col2.y, progress),lerp(col1.z, col2.z, progress),lerp(col1.w, col2.w, progress));
    //mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
    //mat4 object_data = mat4_mul(mat4_translate(v3(0,1 * fabs(sin(5 * dtime_sec(dtime_now()))),-15)), mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7)));
    mat4 object_data[2] = {model, {col.x, col.y, col.z, col.w}};
    dg_set_desc_set(ddev,&ddev->particle_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 4,6);

    //dg_rendering_end(ddev);
}

void dParticleEmitter::delP(u32 index){
    if (this->particle_count)
        this->p[index] = this->p[this->particle_count--];
}

void dParticleEmitter::addP(dParticle p){
    this->p[this->particle_count++] = p;
}

void dParticleEmitter::emit(void){
    dParticle p = {0};
    float dir_x = r01() * 2.0f - 1.0f;
    float dir_y = r01() * 2.0f - 1.0f;
    vec3 vel = v3(dir_x, 1, dir_y);
    vel = vec3_normalize(vel);
    p.vel = vel;
    p.pos = this->world_position;
    p.grav_effect = 1;
    p.scale =1.0f;
    p.life_length= 1.0;
    if (this->particle_count < DPARTICLE_EMITTER_MAX_PARTICLES)
        this->addP(p);
}

void dParticleEmitter::genParticles(f32 dt){
    u32 particles_to_create = (f32)(dt * this->pps);
    for (u32 i = 0; i < particles_to_create; ++i){
        this->emit();
    }
}

void dParticleEmitter::init(void){
    this->particle_count = 0;
    this->pps = 70;
    this->color1 = v4(0.0,0.0,0.0,1.0);
    this->color2 = v4(1.0,1.0,1.0,1.0);
    this->local_position = v3(0,0,0);
}

extern mat4 view, proj;
void dParticleEmitter::render(void){
    //set desc set 0
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(&dd,&dd.particle_pipe, data, sizeof(data), 0);
    dg_rendering_begin(&dd, NULL, 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    for (u32 i =0; i < this->particle_count;++i){
        this->p[i].render(this->color1, this->color2);
    }
    dg_rendering_end(&dd);
}

void dParticleEmitter::update(f32 dt){
    //printf("particle count: %d\n", emitter->particle_count);
    this->genParticles(dt);
    for (u32 i =0; i < this->particle_count;++i){
        if (!this->p[i].update(dt))
            this->delP(i);
    }
}

void dParticleEmitter::deinit(void){
    //Does nothing, for now.
}


dParticleEmitterCM particle_emitter_cm;

//TODO: this allocate should happen after every insert if there is not enough size for new element
void dParticleEmitterCM::allocate(u32 size)
{
    assert(size > this->data.n);

    struct InstanceData new_data;
    const u32 bytes = size * (sizeof(dEntity) + sizeof(dParticleEmitter));
        
    new_data.buffer = dalloc(bytes);
    new_data.n = this->data.n;
    new_data.allocated = size;

    new_data.entity = (dEntity *)(new_data.buffer);
    new_data.emitter = (dParticleEmitter*)(new_data.entity + size);

    if (this->data.buffer != NULL){ //if we have data from previous allocation, copy.
        memcpy(new_data.entity, this->data.entity, this->data.n * sizeof(dEntity));
        memcpy(new_data.emitter, this->data.emitter, this->data.n * sizeof(dParticleEmitter));
    }

    if (this->data.buffer != NULL)
        dfree(this->data.buffer);
    this->data = new_data;
}

void dParticleEmitterCM::init(void){
    memset(this, 0, sizeof(dParticleEmitterCM));
    this->entity_hash = NULL;
    dParticleEmitterCM::allocate(10);
    hmdefault(this->entity_hash, 0xFFFFFFFF);
    dComponentField f1 = dcomponent_field_make("world_position", offsetof(dParticleEmitter, world_position), DCOMPONENT_FIELD_TYPE_VEC3);
    dComponentField f3 = dcomponent_field_make("local_position", offsetof(dParticleEmitter, local_position), DCOMPONENT_FIELD_TYPE_VEC3);
    dComponentField f2 = dcomponent_field_make("pps", offsetof(dParticleEmitter, pps), DCOMPONENT_FIELD_TYPE_U32);
    memset(&this->component_desc, 0, sizeof(this->component_desc));
    dcomponent_desc_insert(&this->component_desc, f1);
    dcomponent_desc_insert(&this->component_desc, f2);
    dcomponent_desc_insert(&this->component_desc, f3);
}


u32 dParticleEmitterCM::add(dEntity e){

    //Find the next index available by the manager and create an EntityID -> index relation
    u32 component_index = this->data.n;
    hmput(this->entity_hash, e.id, component_index);

    //Initialize the component's data in that index
    this->data.entity[component_index] = e;
    this->data.emitter[component_index].init();

    //return the index and increment the global index of the manager, to be ready for the next insertion
    return this->data.n++;
}


//Return value of 0xFFFFFFFF means entity not found
u32 dParticleEmitterCM::lookup(dEntity e){
    u32 component_index = hmget(this->entity_hash, e.id);
    return component_index;
}

void dParticleEmitterCM::del(u32 index){
    u32 last_component = this->data.n-1;
    dEntity e = this->data.entity[index];
    dEntity last_entity = this->data.entity[last_component];

    this->data.entity[index] = this->data.entity[last_component];
    this->data.emitter[index] = this->data.emitter[last_component];

    hmdel(this->entity_hash, e.id);
    hmdel(this->entity_hash, last_entity.id);
    hmput(this->entity_hash, last_entity.id, index);

    this->data.n--;
}

//we update all the emitters
void dParticleEmitterCM::simulate(f32 dt){
    for (u32 i = 0; i < this->data.n; ++i){
        this->data.emitter[i].update(dt);
    }
}

//we render all the emitters
void dParticleEmitterCM::render(void){

    for (u32 i = 0; i < this->data.n; ++i){

        //if there is a transform component for our entity, add that to the position of the emitter
        dEntity current_entity = this->data.entity[i];
        u32 transform_index = dtransform_cm_lookup(&transform_manager, current_entity);
        vec3 transform_pos= v3(0,0,0);
        if(transform_index != DENTITY_NOT_FOUND){
            dTransform lp = transform_manager.data.world[transform_index]; 
            transform_pos = lp.trans;
        }
        this->data.emitter[i].world_position = vec3_add(this->data.emitter[i].local_position, transform_pos);
        this->data.emitter[i].render();
    }
}

dParticleEmitter *dParticleEmitterCM::emitter(u32 index){
    return &this->data.emitter[index];
}