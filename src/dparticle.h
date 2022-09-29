#ifndef __DPARTICLE__
#define __DPARTICLE__
#include "tools.h"
#include "dentity.h"

#define DPARTICLE_GRAVITY 9.8

typedef struct dParticle{
    vec3 pos;
    vec3 vel;
    f32 grav_effect;
    f32 scale;
    f32 rotation;
    f32 life_length;
    f32 elapsed_time;

    b32 update(f32 dt);
    void render(vec4 col1, vec4 col2);
}dParticle;

void dparticle_system_init(void);


#define DPARTICLE_EMITTER_MAX_PARTICLES 512
enum dParticleEmitterType{
    DPARTICLE_EMITTER_TYPE_NONE = 0,
    DPARTICLE_EMITTER_TYPE_SPRINKLE = 1,
    DPARTICLE_EMITTER_TYPE_RING = 2,
    DPARTICLE_EMITTER_TYPE_RANDOM = 3,
    DPARTICLE_EMITTER_TYPE_COUNT,
};

struct dParticleEmitter{
    dParticle p[DPARTICLE_EMITTER_MAX_PARTICLES]; //TODO: make dynamic, its static only to test
    u32 particle_count;
    u32 pps; //particles per second
    vec3 local_position; 
    vec3 world_position; //based on 

    vec4 color1; //source color
    vec4 color2; //dest color
    //maybe put texture slots? (pt.2)

    void init(void);
    void update(f32 dt);
    void render(void); //TODO, maybe pass graphics device or sth????
    void deinit(void);

    void genParticles(f32 dt);
    void emit(void);
    void addP(dParticle p);
    void delP(u32 index);
};

 
typedef struct dParticleEmitterCM {
    struct InstanceData {
        u32 n; // No. of _used_ instances (current)
        u32 allocated; // No. of allocated instances (max)
        void *buffer; // Buffer w/ instance Data

        dEntity *entity;
        dParticleEmitter *emitter;
    };
    InstanceData data;
    struct {u32 key; u32 value;}*entity_hash;//entity ID -> array index

    dComponentDesc component_desc;

    //DOC: inits the particle component manager           
    void              init(void);
    //DOC: simulates all particle emitters (this could be done in parallel)
    void              simulate(f32 dt);
    //DOC: renders every particle emitter's particles
    void              render(void);
    //DOC: adds an entity to the manager and returns its index
    u32               add(dEntity e);
    //DOC: returns the component index of some entity
    u32               lookup(dEntity e);
    //DOC: deletes the particle emitter at index
    void              del(u32 index);
    //DOC: returns the particle emitter for some index
    dParticleEmitter *emitter(u32 index);

    //allocates more emitter slots in the manager
    void allocate(u32 size);
}dParticleEmitterCM;

void dparticle_emitter_cm_init(dParticleEmitterCM *manager);
u32 dparticle_emitter_cm_simulate(dParticleEmitterCM *manager, f32 dt);
u32 dparticle_emitter_cm_render(dParticleEmitterCM *manager);
u32 dparticle_emitter_cm_add(dParticleEmitterCM *manager, dEntity e);
u32 dparticle_emitter_cm_lookup(dParticleEmitterCM *manager, dEntity e);
void dparticle_emitter_cm_del(dParticleEmitterCM *manager, u32 index);
dParticleEmitter *dparticle_emitter_cm_emitter(dParticleEmitterCM *manager,u32 index);


#endif