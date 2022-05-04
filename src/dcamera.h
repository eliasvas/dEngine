#ifndef __DCAMERA_H__
#define __DCAMERA_H__
#include "tools.h"
#include "dinput.h"

typedef enum dCameraMode
{
    DCAM_MODE_LOOK_AT = 1,
    DCAM_MODE_FPS = 2,
    DCAM_MODE_ARCBALL = 3,
}dCameraMode;


typedef struct dCamera
{
    vec3 pos;
    vec3 front;
    vec3 up;
    f32 speed;
    f32 pitch;
    f32 roll;
    f32 yaw;
    dCameraMode mode;
}dCamera;

void dcamera_init(dCamera *camera);
void dcamera_update(dCamera *camera);
mat4 dcamera_get_view_matrix(dCamera *camera);

#endif