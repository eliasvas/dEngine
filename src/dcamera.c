#include "dcamera.h"
f32 cDT = 1.0f/60.0f;

void dcamera_init(dCamera *camera)
{
    camera->pos = v3(0,4,15);
    camera->front = v3(0,0,1);
    camera->up = v3(0,1,0);
    camera->camera_speed = 10.0f;
}

void dcamera_update(dCamera *camera)
{
    if (dkey_down(DK_W))
    {
        camera->pos.y += cDT * camera->camera_speed;
    }
    if (dkey_down(DK_S))
    {
        camera->pos.y -= cDT * camera->camera_speed;
    }
    if (dkey_down(DK_A))
    {
        camera->pos.x -= cDT * camera->camera_speed;
    }
    if (dkey_down(DK_D))
    {
        camera->pos.x += cDT * camera->camera_speed;
    }
}

mat4 dcamera_get_view_matrix(dCamera *camera)
{
    return look_at(camera->pos, vec3_sub(camera->pos, camera->front), camera->up);
}
