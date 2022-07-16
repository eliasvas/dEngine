#include "danim.h"



mat4 calc_joint_matrix(dJointTransform t)
{
    mat4 rotation = m4d(1.0f);
    mat4 translation = m4d(1.0f);
    mat4 scale = m4d(1.0f);
    if (t.flags & DJOINT_FLAG_QUAT)
        rotation = quat_to_mat4(t.quaternion);
    if (t.flags & DJOINT_FLAG_TRANS)
        translation = mat4_translate(t.translation);
    if (t.flags & DJOINT_FLAG_SCALE)
        scale = mat4_scale(t.scale);
    
    return mat4_mul(translation, mat4_mul(rotation, scale));
}

void calc_global_joint_transforms(dJointInfo *j, mat4 parent_transform,dJointTransform* local_joint_transforms, mat4*joint_transforms){
    if (j == NULL)return;
    u32 joint_index = j->id;

    dJointTransform t = local_joint_transforms[j->id];
    mat4 local_joint_transform = calc_joint_matrix(t);

    joint_transforms[joint_index] = mat4_mul(parent_transform,local_joint_transform);
    //joint_transforms[joint_index] = mat4_mul(local_joint_transform, parent_transform);
    for (u32 i = 0; i< j->children_count; ++i)
    {
        calc_global_joint_transforms(j->children[i], joint_transforms[joint_index],local_joint_transforms, joint_transforms);
    }
}

dJointInfo *process_joint_info(cgltf_node *joint, dJointInfo *parent, dSkeletonInfo *info)
{
    if (joint == NULL)return NULL;
    u32 joint_index = hmget(info->name_hash, hash_str(joint->name));
    info->joint_count++;
    dJointInfo *j = &info->joint_hierarchy[joint_index];
    j->parent = parent;
    j->id = joint_index;
    j->children_count = 0;
    //calculate inverse bind matrix for the joint
    dJointTransform jt = {0};
    if (joint->has_rotation){
        jt.quaternion = quat(joint->rotation[0],joint->rotation[1],joint->rotation[2],joint->rotation[3]);
        jt.flags |= DJOINT_FLAG_QUAT;
    }
    if (joint->has_scale){
        jt.scale = v3(joint->scale[0], joint->scale[1], joint->scale[2]);
        jt.flags |= DJOINT_FLAG_SCALE;
    }
    if (joint->has_translation){
        jt.translation = v3(joint->translation[0], joint->translation[1], joint->translation[2]);
        jt.flags |= DJOINT_FLAG_TRANS;
    }
    j->ibm = mat4_inv(calc_joint_matrix(jt));

    for (u32 i = 0; i < joint->children_count; ++i){
        j->children[j->children_count++] = process_joint_info(joint->children[i],j, info);
    }
    return j;
}




dAnimation danim_create(dSkeletonInfo skeleton_info,u32 keyframe_count)
{
    dAnimation anim = {0};
    anim.keyframe_count = keyframe_count;
    anim.skeleton_info = skeleton_info;
    anim.timestamps = malloc(sizeof(f32) * keyframe_count);
    u32 joint_count = anim.skeleton_info.joint_count;
    anim.joint_count = joint_count;
    anim.keyframes = malloc(sizeof(dJointTransform*) * joint_count);
    for (u32 i = 0; i < joint_count; ++i)
    {
        anim.keyframes[i] = malloc(sizeof(dJointTransform) * anim.keyframe_count);
        memset(anim.keyframes[i], 0, sizeof(dJointTransform) * anim.keyframe_count);
    }
    return anim;
}

dAnimation danim_load(cgltf_animation *anim, dSkeletonInfo info)
{
    dAnimation animation = danim_create(info, anim->channels[0].sampler->input->count);
    //first we copy the time offsets
    float *time_offsets0 = anim->channels[0].sampler->input->offset + anim->channels[0].sampler->input->buffer_view->buffer->data + anim->channels[0].sampler->input->buffer_view->offset;
    memcpy(animation.timestamps, time_offsets0, anim->channels[0].sampler->input->count * sizeof(f32));
    animation.timestamps[0] = 0.0f;
    //then all the joint transforms
    for (u32 i = 0; i < anim->channels_count; ++i)
    {
        cgltf_node *node = anim->channels[i].target_node;
        u32 joint_index = hmget(info.name_hash, hash_str(node->name));

        cgltf_animation_path_type type = anim->channels[i].target_path;
        float *time_offsets = anim->channels[i].sampler->input->offset + anim->channels[i].sampler->input->buffer_view->buffer->data + anim->channels[i].sampler->input->buffer_view->offset;
                
        vec4 *quat_offsets = anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset;
        vec3 *trans_offsets = anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset;
        vec3 *scale_offsets = anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset;

        u32 anim_keyframe_count = anim->channels[i].sampler->input->count;
        for (u32 i = 0; i < anim_keyframe_count; ++i)
        {
            //for each keyframe in the channel
            u32 anim_offset = i;
            //we put the keyframe in the correct joint transform in our keyframes array
            dJointTransform *ljt = &animation.keyframes[joint_index][i];
            if (type == cgltf_animation_path_type_rotation){
                ljt->quaternion = quat_add(ljt->quaternion, quat(quat_offsets[anim_offset].x,quat_offsets[anim_offset].y,quat_offsets[anim_offset].z,quat_offsets[anim_offset].w)); 
                ljt->flags |= DJOINT_FLAG_QUAT;
            }
            else if (type == cgltf_animation_path_type_translation){
                ljt->translation = vec3_add(ljt->translation,v3(trans_offsets[anim_offset].x,trans_offsets[anim_offset].y,trans_offsets[anim_offset].z));
                ljt->flags |= DJOINT_FLAG_TRANS;
            }
            else if (type == cgltf_animation_path_type_scale){
                ljt->scale = vec3_add(ljt->scale,v3(scale_offsets[anim_offset].x,scale_offsets[anim_offset].y,scale_offsets[anim_offset].z));
                ljt->flags |= DJOINT_FLAG_SCALE;
            }
        }

    }
    return animation;
}