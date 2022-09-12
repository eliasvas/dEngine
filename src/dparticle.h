#ifndef __DPARTICLE__
#define __DPARTICLE__
#include "tools.h"

#define DPARTICLE_GRAVITY 9.8

typedef struct dParticle{
    vec3 pos;
    vec3 vel;
    f32 grav_effect;
    f32 scale;
    f32 rotation;
    f32 life_length;
    f32 elapsed_time;
}dParticle;

b32 dparticle_update(dParticle *p, f32 dt);

void dparticle_render(dParticle *p);

void dparticle_system_init(void);


#define DPARTICLE_EMITTER_MAX_PARTICLES 512
typedef enum dParticleEmitterType{
    DPARTICLE_EMITTER_TYPE_NONE = 0,
    DPARTICLE_EMITTER_TYPE_SPRINKLE = 1,
    DPARTICLE_EMITTER_TYPE_RING = 2,
    DPARTICLE_EMITTER_TYPE_RANDOM = 3,
    DPARTICLE_EMITTER_TYPE_COUNT,
}dParticleEmitterType;
typedef struct dParticleEmitter{
    dParticle p[DPARTICLE_EMITTER_MAX_PARTICLES]; //make dynamic, its static only to test
    u32 particle_count;
    u32 pps; //particles per second
    vec3 position; //TODO: should be merged with transform components and update pos per frame

    vec4 color1; //source color
    vec4 color2; //dest color
    //maybe put texture slots? (pt.2)
}dParticleEmitter;



void dparticle_emitter_update(dParticleEmitter *emitter, f32 dt);
void dparticle_emitter_render(dParticleEmitter *emitter);

#endif