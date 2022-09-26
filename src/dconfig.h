#ifndef __DCONFIG__
#define __DCONFIG__
#include "tools.h"

enum dgAPI
{
    DG_VULKAN = 1,
    DG_OPENGL = 2,
    DG_SOFTWARE = 3,
    DG_MAX_API,
};

struct dConfig
{
    dgAPI graphics_api;
    vec2 default_resolution;
    char shader_path[64];
    char spirv_path[64];
};

//Fills the (single) engine config struct
//NOTE(ilias): path to dconfig is source dir
void dconfig_load(void);
void dconfig_save(void);
void dconfig_default(void);

#endif