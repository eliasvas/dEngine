#include "danim.h"
//TODO: add Qlerp quaternion interpolation for local joint transforms


dJointTransform djt_default(void)
{
    dJointTransform t;
    t.rot = quat(0,0,0,1);
    t.trans = v3(0.0f,0.0f,0.0f);
    t.scale = v3(1.0f,1.0f,1.0f);
    return t;
}
mat4 calc_joint_matrix(dJointTransform t)
{
    mat4 rotation = quat_to_mat4(t.rot);
    mat4 trans = mat4_translate(t.trans);
    mat4 scale = mat4_scale(t.scale);
    
    return mat4_mul(trans, mat4_mul(rotation, scale));
}

void calc_global_joint_transforms(dJointInfo *j, mat4 parent_transform,dJointTransform* local_joint_transforms, mat4*joint_transforms){
    
    if (j == NULL)
        return;
    u32 joint_index = j->id;

    dJointTransform t = local_joint_transforms[joint_index];
    mat4 local_joint_transform = calc_joint_matrix(t);
    
    joint_transforms[joint_index] = mat4_mul(parent_transform,local_joint_transform);
    for (u32 i = 0; i< j->children_count; ++i)
    {
        calc_global_joint_transforms(j->children[i], joint_transforms[joint_index],local_joint_transforms, joint_transforms);
    }
}

dJointInfo *process_joint_info(cgltf_node *joint, dJointInfo *parent, dSkeletonInfo *info)
{
    if (joint == NULL)return NULL;
    u64 hash_name= hash_str(joint->name);
    u32 joint_index = hmget(info->name_hash, hash_name);
    info->joint_count++;
    dJointInfo *j = &info->joint_hierarchy[joint_index];
    memset(j, 0, sizeof(dJointInfo));
    j->parent = parent;
    j->id = joint_index;
    j->children_count = 0;
    //calculate inverse bind matrix for the joint
    dJointTransform jt = djt_default();
    if (joint->has_rotation)
        jt.rot = quat(joint->rotation[0],joint->rotation[1],joint->rotation[2],joint->rotation[3]);
    
    if (joint->has_scale)
        jt.scale = v3(joint->scale[0], joint->scale[1], joint->scale[2]);
    
    if (joint->has_translation)
        jt.trans = v3(joint->translation[0], joint->translation[1], joint->translation[2]);
    
    j->ibm = mat4_inv(calc_joint_matrix(jt));

    for (u32 i = 0; i < joint->children_count; ++i){
        j->children[j->children_count++] = process_joint_info(joint->children[i],j, info);
    }
    return j;
}




dAnimation danim_create(dSkeletonInfo skeleton_info,u32 keyframe_count)
{
    dAnimation anim = {};
    anim.keyframe_count = keyframe_count;
    anim.skeleton_info = skeleton_info;
    anim.timestamps = (f32*)dalloc(sizeof(f32) * keyframe_count);
    u32 joint_count = anim.skeleton_info.joint_count;
    anim.joint_count = joint_count;
    anim.keyframes = (dJointTransform**)dalloc(sizeof(dJointTransform*) * joint_count);
    for (u32 i = 0; i < joint_count; ++i)
    {
        anim.keyframes[i] = (dJointTransform*)dalloc(sizeof(dJointTransform) * anim.keyframe_count);
        memset(anim.keyframes[i], 0, sizeof(dJointTransform) * anim.keyframe_count);
    }
    return anim;
}

dAnimation danim_load(cgltf_animation *anim, dSkeletonInfo info)
{
    dAnimation animation = danim_create(info, anim->channels[0].sampler->input->count);
    //first we copy the time offsets
    float *time_offsets0 = (f32*)(anim->channels[0].sampler->input->offset + anim->channels[0].sampler->input->buffer_view->buffer->data + anim->channels[0].sampler->input->buffer_view->offset);
    memcpy(animation.timestamps, time_offsets0, anim->channels[0].sampler->input->count * sizeof(f32));
    animation.timestamps[0] = 0.0f;
    //then all the joint transforms
    for (u32 i = 0; i < anim->channels_count; ++i)
    {
        cgltf_node *node = anim->channels[i].target_node;
        u64 name_hash =hash_str(node->name);
        u32 joint_index = hmget(info.name_hash, name_hash);

        cgltf_animation_path_type type = anim->channels[i].target_path;
        float *time_offsets = (f32*)(anim->channels[i].sampler->input->offset + anim->channels[i].sampler->input->buffer_view->buffer->data + anim->channels[i].sampler->input->buffer_view->offset);
                
        vec4 *quat_offsets = (vec4*)(anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset);
        vec3 *trans_offsets = (vec3*)(anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset);
        vec3 *scale_offsets = (vec3*)(anim->channels[i].sampler->output->offset + anim->channels[i].sampler->output->buffer_view->buffer->data + anim->channels[i].sampler->output->buffer_view->offset);

        u32 anim_keyframe_count = anim->channels[i].sampler->input->count;
        for (u32 i = 0; i < anim_keyframe_count; ++i)
        {
            //for each keyframe in the channel
            u32 anim_offset = i;
            //we put the keyframe in the correct joint transform in our keyframes array
            dJointTransform *ljt = &animation.keyframes[joint_index][i];
            if (type == cgltf_animation_path_type_rotation){
                ljt->rot = quat_add(ljt->rot, quat(quat_offsets[anim_offset].x,quat_offsets[anim_offset].y,quat_offsets[anim_offset].z,quat_offsets[anim_offset].w)); 
            }
            else if (type == cgltf_animation_path_type_translation){
                ljt->trans = vec3_add(ljt->trans,v3(trans_offsets[anim_offset].x,trans_offsets[anim_offset].y,trans_offsets[anim_offset].z));
            }
            else if (type == cgltf_animation_path_type_scale){
                ljt->scale = vec3_add(ljt->scale,v3(scale_offsets[anim_offset].x,scale_offsets[anim_offset].y,scale_offsets[anim_offset].z));
            }
        }

    }
    return animation;
}


//#include "dmodel.h"
dAnimator danimator_init(dModel *m, dAnimation *a, mat4 *ibm, u32 anim_length){
    dAnimator anim = {};
    //anim.model = m;
    anim.anim = a;
    anim.current_time = 0;
    anim.start_time = dtime_sec(dtime_now());
    anim.animation_speed = 25.0f;
    u32 joint_count = a->joint_count;
    anim.ljt = (dJointTransform*)dalloc(sizeof(dJointTransform) * joint_count*2);
    anim.gjm = (mat4*)dalloc(sizeof(mat4) * joint_count*2);
    anim.ibm = (mat4*)dalloc(sizeof(mat4) * joint_count*2);

    for (u32 i = 0; i < joint_count; ++i){
        anim.ljt[i] = djt_default();
        anim.gjm[i] = m4d(1.0f);
        anim.ibm[i] = ibm[i];
    }
    return anim;
}

void danimator_animate(dAnimator *animator)
{

    animator->current_time = (animator->animation_speed * (dtime_sec(dtime_now()) - animator->start_time));
    
    f32 a = fmod(animator->current_time, animator->anim->keyframe_count);
    //we extract the fractional part
    a = a-(long)a;
    
    u32 kf0 = (u32)animator->current_time % animator->anim->keyframe_count;
    u32 kf1 = (kf0+1)  % animator->anim->keyframe_count;
    for (u32 i = 0; i < animator->anim->skeleton_info.joint_count;++i)
    {
        u32 joint_index = animator->anim->skeleton_info.joint_hierarchy[i].id;
        dJointTransform tkf0 = animator->anim->keyframes[joint_index][kf0];
        dJointTransform tkf1 = animator->anim->keyframes[joint_index][kf1];
        tkf0.rot = quat_normalize(tkf0.rot);
        tkf1.rot = quat_normalize(tkf1.rot);
        dJointTransform interp;
        interp.trans = vec3_lerp(tkf0.trans, tkf1.trans, a);
        interp.scale = vec3_lerp(tkf0.scale, tkf1.scale, a);
        interp.rot = nlerp(tkf0.rot, tkf1.rot, a);
        //https://stackoverflow.com/questions/42428136/rot-is-flipping-sign-for-very-similar-rotations
        if (quat_mul(tkf0.rot, quat_conj(tkf1.rot)).w < 0)
            interp.rot = nlerp(tkf0.rot, tkf1.rot, -a);
        //printf("a: %f\n", a);
        animator->ljt[joint_index] = interp;
    }
    
    //*/
    
    calc_global_joint_transforms(&animator->anim->skeleton_info.joint_hierarchy[0], m4d(1.0f), animator->ljt, animator->gjm);
    for (u32 i = 0; i < animator->anim->joint_count; ++i){
        dJointInfo *j = &animator->anim->skeleton_info.joint_hierarchy[i];
        u32 joint_index = j->id;
        animator->gjm[j->id] = mat4_mul(animator->gjm[j->id], animator->ibm[j->id]);
        //animator->gjm[j->id] = m4d(1.0f);
    }
    //animator->gjm[0] = m4d(0.0); //this kindof works, but why?????

}



dAnimSocket danimator_make_socket(dAnimator *animator,char *joint_name, mat4 local_transform){
    dAnimSocket s = {};
    u64 name_hash = hash_str(joint_name);
    u32 joint_index = hmget(animator->anim->skeleton_info.name_hash, name_hash);
    s.joint_id = joint_index;
    s.local_transform = local_transform;
    return s;
}

mat4 danimator_get_socket_transform(dAnimator * animator, dAnimSocket s){
    mat4 gjm = mat4_mul(animator->gjm[s.joint_id],mat4_inv(animator->ibm[s.joint_id]));
    return mat4_mul(animator->model_mat, mat4_mul( s.local_transform,gjm));
}

