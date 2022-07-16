#ifndef __DMODEL__
#define __DMODEL__
#include "dgfx.h"
#include "dmaterial.h"
#include "stb/stb_ds.h"
#define DMODEL_MAX_TEXTURES 8
#define DMODEL_MAX_MESHES_PER_MODEL 8

typedef struct dJointInfo dJointInfo;
struct dJointInfo{
    u32 id;
    dJointInfo *parent;
    dJointInfo *children[8];
    u32 children_count;
};
#define MAX_JOINT_COUNT 32
typedef struct dSkeletonInfo
{
    struct {u64 key; u32 value}*name_hash;//name -> ID
    dJointInfo joint_hierarchy[32];
    u32 joint_count;
}dSkeletonInfo;

typedef enum dJointFlagsS{
    DJOINT_FLAG_QUAT = 0x1,
    DJOINT_FLAG_TRANS = 0x2,
    DJOINT_FLAG_SCALE = 0x4,
}dJointFlags;
typedef struct dJointTransform{
    Quaternion quaternion;
    vec3 translation;
    vec3 scale;
    dJointFlags flags;
}dJointTransform;

typedef struct dMesh {
    dgBuffer pos_buf;
    dgBuffer norm_buf;
    dgBuffer tex_buf;
    dgBuffer tang_buf;
    dgBuffer joint_buf;
    dgBuffer weight_buf;

    dSkeletonInfo skeleton_info;

    dgBuffer index_buf;
}dMesh;

#define MAX_BONES_PER_VERTEX 4
typedef struct dVertexBoneData {
    u32 boneIDs[MAX_BONES_PER_VERTEX];
    u32 weights[MAX_BONES_PER_VERTEX];
}dVertexBoneData;



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