#include "dcamera.h"
f32 cDT = 1.0f/60.0f;

void dcamera_init(dCamera *camera)
{
    camera->pos = v3(0,4,15);
    camera->front = v3(0,0,-1);
    camera->up = v3(0,1,0);
    camera->speed = 10.0f;
    camera->mode = DCAM_MODE_FPS;
    //camera->mode = DCAM_MODE_LOOK_AT;
    camera->pitch = 0;
    camera->roll = 0;
    camera->yaw = 0;
}

//right handed fps
mat4 fps_view( vec3 eye, float pitch, float yaw )
{
    f32 cos_pitch = cos(to_radians(pitch));
    f32 sin_pitch = sin(to_radians(pitch));
    f32 cos_yaw = cos(to_radians(yaw));
    f32 sin_yaw = sin(to_radians(yaw));
 
    vec3 xaxis = { cos_yaw, 0, -sin_yaw };
    vec3 yaxis = { sin_yaw * sin_pitch, cos_pitch, cos_yaw * sin_pitch };
    vec3 zaxis = { sin_yaw * cos_pitch, -sin_pitch, cos_pitch * cos_yaw };
 
    mat4 view_matrix = {
              xaxis.x,yaxis.x,zaxis.x,0,xaxis.y,yaxis.y,zaxis.y,0,
              xaxis.z,yaxis.z,zaxis.z,0,-vec3_dot( xaxis, eye ), 
              -vec3_dot( yaxis, eye ), -vec3_dot( zaxis, eye ),1};
    
    return view_matrix;
}

void dcamera_update(dCamera *camera)
{
    if(camera->mode == DCAM_MODE_FPS)
    {
        camera->pitch += dkey_down(DK_RMB) * cDT * camera->speed * dinput_get_mouse_delta().y; 
        camera->yaw += -dkey_down(DK_RMB) * cDT * camera->speed * dinput_get_mouse_delta().x; 
    }
    if (dkey_down(DK_W))
        camera->pos = vec3_add(camera->pos, vec3_mulf(camera->front, -cDT * camera->speed));
    if (dkey_down(DK_S))
        camera->pos = vec3_add(camera->pos, vec3_mulf(camera->front, cDT * camera->speed));
    if (dkey_down(DK_A))
        camera->pos = vec3_add(camera->pos, vec3_mulf(vec3_cross(camera->up,camera->front), -cDT * camera->speed));
    if (dkey_down(DK_D))
        camera->pos = vec3_add(camera->pos, vec3_mulf(vec3_cross(camera->up,camera->front), cDT * camera->speed));
}

mat4 dcamera_get_view_matrix(dCamera *camera)
{
    if (camera->mode == DCAM_MODE_FPS)
    {
        mat4 ret = fps_view(camera->pos, camera->pitch, camera->yaw);
        camera->front = v3(ret.raw[2], ret.raw[6], ret.raw[10]);
        return ret;
    }
    return look_at(camera->pos, vec3_add(camera->pos,camera->front), camera->up);
}
