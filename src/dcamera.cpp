#include "dcamera.h"
#include "tools.h"

//FIX: can't turn while moving?????

void dCamera::init(void)
{
    this->pos = v3(0,4,15);
    this->front = v3(0,0,-1);
    this->up = v3(0,1,0);
    this->speed = 10.0f;
    this->mode = DCAM_MODE_FPS;
    //camera->mode = DCAM_MODE_LOOK_AT;
    this->pitch = 0;
    this->roll = 0;
    this->yaw = 0;
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

void dCamera::update(f64 dt)
{
    if(this->mode == DCAM_MODE_FPS)
    {
        this->pitch += dkey_down(DK_RMB) * dt * this->speed * dinput_get_mouse_delta().y; 
        this->yaw += -dkey_down(DK_RMB) * dt * this->speed * dinput_get_mouse_delta().x; 
    }
    if (dkey_down(DK_W))
        this->pos = vec3_add(this->pos, vec3_mulf(this->front, -dt * this->speed));
    if (dkey_down(DK_S))
        this->pos = vec3_add(this->pos, vec3_mulf(this->front, dt * this->speed));
    if (dkey_down(DK_A))
        this->pos = vec3_add(this->pos, vec3_mulf(vec3_cross(this->up,this->front), -dt * this->speed));
    if (dkey_down(DK_D))
        this->pos = vec3_add(this->pos, vec3_mulf(vec3_cross(this->up,this->front), dt * this->speed));
}

mat4 dCamera::getViewMatrix(void)
{
    if (this->mode == DCAM_MODE_FPS)
    {
        mat4 ret = fps_view(this->pos, this->pitch, this->yaw);
        this->front = v3(ret.raw[2], ret.raw[6], ret.raw[10]);
        return ret;
    }
    return look_at(this->pos, vec3_add(this->pos,this->front), this->up);
}
