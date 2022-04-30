#ifndef __DCAMERA_H__
#define __DCAMERA_H__
#include "tools.h"
#include "dinput.h"

typedef struct dCamera
{
    vec3 pos;
    vec3 front;
    vec3 up;
    f32 camera_speed;
}dCamera;

void dcamera_init(dCamera *camera);
void dcamera_update(dCamera *camera);
mat4 dcamera_get_view_matrix(dCamera *camera);

#endif