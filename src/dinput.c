#include "dinput.h"
#include "tools.h"
#include "dprofiler.h"
#include "dwin.h" //we need GLFW window handle to poll input
#include "../ext/glfw/include/GLFW/glfw3.h"
 
extern dWindow main_window;
//The input state of the engine for the current frame (its a singleton!)
typedef struct dInputState
{
    dKey keys[DK_MAX];
    dKey prev_keys[DK_MAX];

    f32 mouse_pos_x, mouse_pos_y;
    f32 mouse_delta_x, mouse_delta_y;
}dInputState;

//This is the input singleton
dInputState dis;

void dinput_init(void)
{
    memset(&dis.keys, 0, DK_MAX * sizeof(dis.keys[0]));
    memset(&dis.prev_keys, 0, DK_MAX * sizeof(dis.keys[0]));
}

void dinput_update(void)
{
    DPROFILER_START("input_update");
    glfwPollEvents();
    if (glfwWindowShouldClose(main_window.gwindow))exit(1);
    memcpy(dis.prev_keys,dis.keys, sizeof(dis.keys[0]) * DK_MAX);
    //get keys
    for (u32 i = GLFW_KEY_A; i <= GLFW_KEY_Z; ++i)
    {
        s32 state = glfwGetKey(main_window.gwindow, i);
        if (state == GLFW_PRESS)
            dis.keys[DK_A + (i - GLFW_KEY_A)] = 1;
        else if (state == GLFW_RELEASE)
            dis.keys[DK_A + (i - GLFW_KEY_A)] = 0;

    }
    //get numbers
    for (u32 i = GLFW_KEY_0; i <= GLFW_KEY_9; ++i)
    {
        s32 state = glfwGetKey(main_window.gwindow, i);
        if (state == GLFW_PRESS)
            dis.keys[DK_0 + (i - GLFW_KEY_0)] = 1;
        else if (state == GLFW_RELEASE)
            dis.keys[DK_0 + (i - GLFW_KEY_0)] = 0;

    }

    
    //get mouse pos
    f64 new_mx, new_my;
    glfwGetCursorPos(main_window.gwindow, &new_mx, &new_my);
    dis.mouse_delta_x = new_mx - dis.mouse_pos_x;
    dis.mouse_delta_y = new_my - dis.mouse_pos_y;
    dis.mouse_pos_x = new_mx;
    dis.mouse_pos_y = new_my;
    //get mouse buttons
    for (u32 i = GLFW_MOUSE_BUTTON_LEFT; i <= GLFW_MOUSE_BUTTON_MIDDLE; ++i)
    {
        s32 state = glfwGetMouseButton(main_window.gwindow, i);
        if (state == GLFW_PRESS)
            dis.keys[DK_LMB + (i - GLFW_MOUSE_BUTTON_LEFT)] = 1;
        else if (state == GLFW_RELEASE)
            dis.keys[DK_LMB + (i - GLFW_MOUSE_BUTTON_LEFT)] = 0;

    }
    DPROFILER_END();
}

vec2 dinput_get_mouse_pos(void)
{
    return (vec2){dis.mouse_pos_x, dis.mouse_pos_y};
}
vec2 dinput_get_mouse_delta(void)
{
    return (vec2){dis.mouse_delta_x, dis.mouse_delta_y};
}
b32 dkey_pressed(dKey k)
{
    return (dis.keys[k] && (dis.prev_keys[k] == 0));
}

b32 dkey_released(dKey k)
{
    return (dis.prev_keys[k] && (dis.keys[k] == 0));
}

b32 dkey_up(dKey k)
{
    return (dis.keys[k] == 0);
}

b32 dkey_down(dKey k)
{
    return (dis.keys[k]);
}

