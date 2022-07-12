#ifndef __DMODEL__
#define __DMODEL__
#include "dgfx.h"
#include "dmaterial.h"
#define DMODEL_MAX_TEXTURES 8
#define DMODEL_MAX_MESHES_PER_MODEL 8

typedef struct dMesh
{
    dgBuffer pos_buf;
    dgBuffer norm_buf;
    dgBuffer tex_buf;
    dgBuffer tang_buf;
    dgBuffer index_buf;
    dgBuffer bone_buf;
}dMesh;

#define DMODEL_BASE_COLOR_INDEX 0
#define DMODEL_ORM_INDEX 1
#define DMODEL_NORMAL_INDEX 2
#define DMODEL_EMISSIVE_INDEX 3
typedef struct dModel
{
    //dgBuffer vertex_buffer;
    dMesh meshes[DMODEL_MAX_MESHES_PER_MODEL];
    u32 meshes_count;

    dgTexture textures[DMODEL_MAX_TEXTURES];
    u32 textures_count;

    b32 finished_loading; //maybe this should be a thing in the asset pipeline
    dMaterial material;
}dModel;
dModel dmodel_load_gltf(const char *filename);

#endif