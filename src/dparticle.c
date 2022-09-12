#include "dparticle.h"
#include "dgfx.h"
#include "deditor.h"

dParticleEmitter test_emitter;


//Initializes the buffers needed to render and update particles
void dparticle_system_init(void){
    dparticle_emitter_init(&test_emitter);
}


//if TRUE particle is fine, if FALSE simulation has finished
b32 dparticle_update(dParticle *p, f32 dt) {
    p->vel.y += DPARTICLE_GRAVITY * p->grav_effect * dt;
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
extern dgBuffer base_ibo;
void dparticle_render(dParticle *p){
    dgDevice *ddev = &dd;
    dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev, main_editor.viewport.x,main_editor.viewport.y,main_editor.viewport.z-main_editor.viewport.x, main_editor.viewport.w -main_editor.viewport.y);
    dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_bind_pipeline(ddev, &ddev->base_pipe);
    mat4 model= mat4_translate(p->pos);
    dgBuffer buffers[] = {base_vbo};
    u64 offsets[] = {0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);

    //mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
    //mat4 object_data = mat4_mul(mat4_translate(v3(0,1 * fabs(sin(5 * dtime_sec(dtime_now()))),-15)), mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7)));
    mat4 object_data[2] = {model, {1.0,1.0,1.0,1.1,0.0,1.0}};
    dg_set_desc_set(ddev,&ddev->base_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 4,6);

    dg_rendering_end(ddev);
}

void dparticle_emitter_init(dParticleEmitter *emitter){
    emitter->particle_count = 0;
    emitter->pps = 70;
    emitter->color1 = v4(1.0,0.3,0.3,1.0);
    emitter->color2 = v4(0.3,0.4,1.0,1.0);
    emitter->position = v3(-5,5,0);
}

extern mat4 view, proj;
void dparticle_emitter_render(dParticleEmitter *emitter){
    //set desc set 0
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(&dd,&dd.def_pipe, data, sizeof(data), 0);
    for (u32 i =0; i < emitter->particle_count;++i){
        dparticle_render(&emitter->p[i]);
    }
}

void dparticle_emitter_del(dParticleEmitter *emitter, u32 index){
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
    p.pos = emitter->position;
    p.grav_effect = 1;
    p.scale =1.0f;
    p.life_length= 4.25;
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
    printf("particle count: %d\n", emitter->particle_count);
    dparticle_emitter_gen_particles(emitter, dt);
    for (u32 i =0; i < emitter->particle_count;++i){
        if (!dparticle_update(&emitter->p[i], dt))
            dparticle_emitter_del(emitter, i);
    }
}
