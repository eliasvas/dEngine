#ifndef __DWINDOW__
#define __DWINDOW__
#include "tools.h"

//#define GLFW_INCLUDE_VULKAN
//#define GLFW_VULKAN_STATIC
#include "../ext/glfw/include/GLFW/glfw3.h"


typedef struct dWindow
{
    GLFWwindow *gwindow;


    u32 width, height;
    b32 hidden;
    b32 resizable;
    b32 resized;
    b32 fullscreen;
    char title[32];
}dWindow;

typedef enum dWindowOptions
{
    DWINDOW_OPT_VULKAN = 0x1,
    DWINDOW_OPT_RESIZABLE = 0x2,
    DWINDOW_OPT_HIDDEN = 0x4
}dWindowOptions;

b32 dwindow_create(dWindow *dw, char *t, u32 w, u32 h, dWindowOptions opt);

#endif