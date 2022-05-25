#ifndef __DMODEL__
#define __DMODEL__
#include "dgfx.h"
#include "dmaterial.h"
#define DMODEL_MAX_TEXTURES 8

//material??
typedef struct dModel
{
    //dgBuffer vertex_buffer;
    dgBuffer pos_buf;
    dgBuffer norm_buf;
    dgBuffer tex_buf;
    //dgBuffer index_buffer;
    dMaterial material;
    dgTexture textures[DMODEL_MAX_TEXTURES];
    u32 textures_count;
    b32 finished_loading; //maybe this should be a thing in the asset pipeline
}dModel;

dModel dmodel_load_gltf(const char *filename);

#endif