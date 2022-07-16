#ifndef __D_ANIM__
#define __D_ANIM__
#include "tools.h"
#include "cgltf/cgltf.h"
#include "dtime.h"
#include "stb/stb_ds.h"

typedef struct dJointInfo dJointInfo;
struct dJointInfo{
    u32 id;
    dJointInfo *parent;
    dJointInfo *children[8];
    u32 children_count;
    mat4 ibm;
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


//   __keyframes__
//│joint  ┌─► keyframe
//▼index  └─► count

typedef struct dAnimation{
    dSkeletonInfo skeleton_info;
    f32 *timestamps;
    dJointTransform **keyframes;
    u32 keyframe_count;
    u32 joint_count;
    //dAllocator alloc;
}dAnimation;



//Takes a Joint Transform and calculates the equivalent Transform Matrix
mat4 calc_joint_matrix(dJointTransform t);

//It take an array of local Joint Transforms, and concatinates them to global Joint Transforms (as matrices)
void calc_global_joint_transforms(dJointInfo *j, mat4 parent_transform,dJointTransform* local_joint_transforms, mat4*joint_transforms);

//Takes the root node of a Mesh and computes its Skeletal Info (mainly the Joint Hierarchy)
dJointInfo *process_joint_info(cgltf_node *joint, dJointInfo *parent, dSkeletonInfo *info);


dAnimation danim_load(cgltf_animation *anim, dSkeletonInfo info);

#endif