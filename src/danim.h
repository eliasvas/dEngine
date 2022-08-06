#ifndef __D_ANIM__
#define __D_ANIM__
#include "tools.h"
#include "cgltf/cgltf.h"
#include "dtime.h"
#include "stb/stb_ds.h"
#include "dentity.h" //mainly for dTransform

typedef dTransform dJointTransform;

typedef struct dJointInfo dJointInfo;
struct dJointInfo{
    u32 id;
    dJointInfo *parent;
    dJointInfo *children[8];
    u32 children_count;
    mat4 ibm;
};
#define MAX_JOINT_COUNT 70
typedef struct dSkeletonInfo
{
    struct {u64 key; u32 value}*name_hash;//name -> ID
    dJointInfo joint_hierarchy[MAX_JOINT_COUNT];
    u32 joint_count;
}dSkeletonInfo;



typedef enum DANIM_INTERP_TYPE{
    DANIM_INTERP_TYPE_STEP = 0x1,
    DANIM_INTERP_TYPE_LINEAR = 0x2,
    DANIM_INTERP_TYPE_CUBIC = 0x4,
}DANIM_INTERP_TYPE;

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

dJointTransform djt_default(void);


//Takes a Joint Transform and calculates the equivalent Transform Matrix
mat4 calc_joint_matrix(dJointTransform t);

//It take an array of local Joint Transforms, and concatinates them to global Joint Transforms (as matrices)
void calc_global_joint_transforms(dJointInfo *j, mat4 parent_transform,dJointTransform* local_joint_transforms, mat4*joint_transforms);

//Takes the root node of a Mesh and computes its Skeletal Info (mainly the Joint Hierarchy)
dJointInfo *process_joint_info(cgltf_node *joint, dJointInfo *parent, dSkeletonInfo *info);


dAnimation danim_load(cgltf_animation *anim, dSkeletonInfo info);


typedef struct dModel dModel;
typedef struct dAnimator{
    dModel *model;
    dAnimation *anim;

    f32 start_time; //when the animator started playing
    f32 current_time;

    f32 animation_speed;

    dJointTransform *ljt;
    mat4 *gjm;
    mat4 *ibm;

    mat4 model_mat;
    //dAllocator alloc;
}dAnimator;

dAnimator danimator_init(dModel *m, dAnimation *a, mat4 *ibm, u32 anim_length);
void danimator_animate(dAnimator *animator);


typedef struct dAnimSocket{
    mat4 local_transform;//transform rel to bone
    u32 joint_id;
}dAnimSocket;

dAnimSocket danimator_make_socket(dAnimator *animator,char *joint_name, mat4 local_transform);

mat4 danimator_get_socket_transform(dAnimator * animator, dAnimSocket s);

#endif