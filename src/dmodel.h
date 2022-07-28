#ifndef __DMODEL__
#define __DMODEL__
#include "dgfx.h"
#include "dmaterial.h"
#include "stb/stb_ds.h"
#include "danim.h"
#define DMODEL_MAX_TEXTURES 8
#define DMODEL_MAX_MESHES_PER_MODEL 10
#define DMODEL_MAX_MESH_PRIMITIVES_PER_MESH 10

typedef struct dMeshPrimitive{
    //offsets encode the [start, finish] within the 
    //component's buffer where the vertex data are for the primitive
    ivec2 pos_offset;
    ivec2 norm_offset;
    ivec2 tex_offset;
    ivec2 tang_offset;
    ivec2 col_offset;
    ivec2 joint_offset;
    ivec2 weight_offset;

    ivec2 index_offset;

    u32 primitive_type;
}dMeshPrimitive;

typedef struct dMesh {
    dgBuffer pos_buf; //GPU buffer storing vertex positions
    dgBuffer norm_buf; //GPU buffer storing vertex normals
    dgBuffer tex_buf; //GPU buffer storing vertex tex-coords
    dgBuffer tang_buf; //GPU buffer storing vertex tangent vectors
    dgBuffer col_buf; //GPU buffer storing vertex colors
    dgBuffer joint_buf; //GPU buffer storing per-vertex joint IDs
    dgBuffer weight_buf; //GPU buffer storing per-vertex joint weight

    dMeshPrimitive primitives[DMODEL_MAX_MESH_PRIMITIVES_PER_MESH];
    u32 primitives_count;

    dSkeletonInfo skeleton_info;

    dgBuffer index_buf;
}dMesh;

//indexes for textures, TODO tidy up the model texture system (and generally the texture manager :P)
#define DMODEL_BASE_COLOR_INDEX 0
#define DMODEL_ORM_INDEX 1
#define DMODEL_NORMAL_INDEX 2
#define DMODEL_EMISSIVE_INDEX 3
typedef struct dModel {
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