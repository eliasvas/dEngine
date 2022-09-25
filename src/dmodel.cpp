#include "dmodel.h"
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf/cgltf.h"
#include "dprofiler.h"
#include "dlog.h"
#include "deditor.h"
//TODO: check if primitive.attributes[weight_index].data->buffer_view->size is too big (it contains everything?)
//and if so, add all of its components sizes and make a new packed array as it should be for each attrib

extern dgDevice dd;

static dAnimation animation;
static dAnimator animator;
extern dEditor main_editor;


extern dgRT csm_rt;//TODO: fix all these externs
extern dgRT composition_rt;

dModel dmodel_load_gltf(const char *filename)
{
    char filepath[256];
    sprintf(filepath,"../assets/%s/%s.gltf",filename,filename);
    dlog(NULL, "gltf FILEPATH: %s\n", filepath);
    dModel model = {};

    cgltf_options options = {};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);


    

    //sprintf(filepath,"../assets/%s/%s.bin",filename,filename);
    if (cgltf_load_buffers(&options, data, filepath) != cgltf_result_success)
    {
        model.finished_loading = 0;
        dlog(NULL, "couldnt load gltf data\n");
        return model;
    }
    if (result != cgltf_result_success)
    {
        model.finished_loading = 0;
        return model;
    }
    //printf("Attr[0] = %s\n",data->meshes[0].primitives[0].attributes[0].name);

    //first load all the textures!
    model.textures_count= data->textures_count;

    dgTexture empty_tex = dg_create_texture_image_wdata(&dd,NULL, 64,64, DG_IMAGE_FORMAT_RGBA8_SRGB,1,1);
    u32 meshes_count = data->meshes_count;
    
    cgltf_primitive primitive = data->meshes[0].primitives[0];

    dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
                &model.gpu_buf,primitive.attributes[0].data->buffer_view->buffer->size,(char*)primitive.attributes[0].data->buffer_view->buffer->data);
                

    for (u32 i = 0; i< meshes_count && i < DMODEL_MAX_MESHES_PER_MODEL; ++i)
    {
        dMesh mesh = {};
        u32 mesh_index = i;
        
        
        for (u32 p= 0; p < data->meshes[mesh_index].primitives_count&& p < DMODEL_MAX_MESH_PRIMITIVES_PER_MESH; ++p){
            cgltf_primitive primitive = data->meshes[mesh_index].primitives[p];
            dMeshPrimitive prim = {};
            s32 norm_index = -1;
            s32 pos_index = -1;
            s32 tex_index = -1;
            s32 joint_index = -1;
            s32 weight_index = -1;
            s32 tangent_index = -1;
            s32 col_index = -1;
            for (u32 j = 0; j < primitive.attributes_count; ++j)
            {
                if (strncasecmp("TEX", primitive.attributes[j].name,3) == 0)
                    tex_index = j;
                else if (strncasecmp("NORM", primitive.attributes[j].name,4) == 0)
                    norm_index = j;
                else if (strncasecmp("POS", primitive.attributes[j].name,3) == 0)
                    pos_index = j;
                else if (strncasecmp("TAN", primitive.attributes[j].name,3) == 0)
                    tangent_index = j;
                else if (strncasecmp("JOINT", primitive.attributes[j].name,5) == 0)
                    joint_index = j;
                else if (strncasecmp("WEI", primitive.attributes[j].name,3) == 0)
                    weight_index = j;
                else if (strncasecmp("COL", primitive.attributes[j].name,3) == 0)
                    col_index = j;
            }

            for  (u32 i = 0; i < 4; ++i)
                prim.m.textures[i] = empty_tex;
            if (primitive.material->has_pbr_metallic_roughness )
            {
                prim.m.settings = (dMaterialSettings)(prim.m.settings |((int)DMATERIAL_BASE_COLOR | (int)DMATERIAL_ORM));
                if (primitive.material->pbr_metallic_roughness.base_color_texture.texture){
                    sprintf(filepath, "../assets/%s/%s", filename,primitive.material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
                    prim.m.textures[DMATERIAL_BASE_COLOR_INDEX] = *(dtexture_manager_add_tex(NULL, filepath, DG_IMAGE_FORMAT_RGBA8_SRGB));
                }
                if (primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture){
                    sprintf(filepath, "../assets/%s/%s", filename,primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri);
                    prim.m.textures[DMATERIAL_ORM_INDEX] = *(dtexture_manager_add_tex(NULL, filepath,DG_IMAGE_FORMAT_RGBA8_UNORM));
                }
                f32 *c = (f32*)&primitive.material->pbr_metallic_roughness.base_color_factor;
                prim.m.col = v4(c[0],c[1],c[2],c[3]);
            }
            if (primitive.material->normal_texture.texture)
            {
                prim.m.settings |= DMATERIAL_NORMAL;
                sprintf(filepath, "../assets/%s/%s", filename,primitive.material->normal_texture.texture->image->uri);
                prim.m.textures[DMATERIAL_NORMAL_INDEX] = *(dtexture_manager_add_tex(NULL, filepath,DG_IMAGE_FORMAT_RGBA8_UNORM));
            }


            if (pos_index != -1){
                u32 offset = primitive.attributes[pos_index].data->offset+ primitive.attributes[pos_index].data->buffer_view->offset;
                u32 size = primitive.attributes[pos_index].data->count * sizeof(vec3);
                prim.pos_offset = iv2(offset, size);
            }
            
            if (tex_index != -1){
                u32 offset = primitive.attributes[tex_index].data->offset+ primitive.attributes[tex_index].data->buffer_view->offset;
                u32 size = primitive.attributes[tex_index].data->count * sizeof(vec2);
                prim.tex_offset = iv2(offset, size);
            }

            if (norm_index != -1){
                u32 offset = primitive.attributes[norm_index].data->offset+ primitive.attributes[norm_index].data->buffer_view->offset;
                u32 size = primitive.attributes[norm_index].data->count * sizeof(vec3);
                prim.norm_offset = iv2(offset, size);
            }

            if (tangent_index != -1){
                u32 offset = primitive.attributes[tangent_index].data->offset+ primitive.attributes[tangent_index].data->buffer_view->offset;
                u32 size = primitive.attributes[tangent_index].data->count * sizeof(vec4);
                prim.tang_offset = iv2(offset, size);
            }

            if (weight_index != -1){
                u32 offset = primitive.attributes[weight_index].data->offset + primitive.attributes[weight_index].data->buffer_view->offset;
                u32 size = primitive.attributes[weight_index].data->count * sizeof(vec4);
                prim.weight_offset = iv2(offset, size);
            }
            

            u32 *j32 = NULL;
            //TODO: SUUUUUUUUUUUUUPER UGLY FIX ASAP, ALSO MEMLEAKS HERE :)))))))))))
            if (joint_index != -1 && !mesh.joint_buf.active)
            {
                u8 *j8 = (u8*)primitive.attributes[joint_index].data->buffer_view->buffer->data + primitive.attributes[joint_index].data->buffer_view->offset + primitive.attributes[joint_index].data->offset;
            
                j32 = (u32*)dalloc(primitive.attributes[joint_index].data->count * sizeof(vec4));
                for (u32 i = 0; i < primitive.attributes[joint_index].data->count*4;++i)
                {
                    j32[i] = j8[i];
                }
                //create joint buffer 
                dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
                &mesh.joint_buf,primitive.attributes[joint_index].data->count * sizeof(vec4),j32);
                dfree(j32);
            }
            if (joint_index != -1){
                u32 offset = 0;//primitive.attributes[joint_index].data->offset;
                u32 size = primitive.attributes[joint_index].data->count * sizeof(vec4);
                prim.joint_offset = iv2(offset, size);
            }

            //CHECK CHECK CHECK ERRROROROROE ERROR
            if (primitive.indices){
                u32 offset = primitive.indices->offset + primitive.indices->buffer_view->offset;
                u32 size = primitive.indices->count *sizeof(u16);
                prim.index_offset = iv2(offset, size);
            }
            mesh.primitives[mesh.primitives_count++] = prim;
        }
        

        
        

        model.meshes[model.meshes_count++] = mesh;

    }
    dSkeletonInfo info = {};
    if (data->animations_count){
        mat4 *ibm = (mat4*)(data->skins[0].inverse_bind_matrices->buffer_view->buffer->data + data->skins[0].inverse_bind_matrices->buffer_view->offset + data->skins[0].inverse_bind_matrices->offset);
        cgltf_node *root_joint = data->skins[0].joints[0];
        
        //first we fill the name hash so we know what bone has what index
        for (u32 i = 0; i < data->skins[0].joints_count;++i)
        {
            cgltf_node *joint = data->skins[0].joints[i];
            u32 joint_index = i;
            u32 hash_name = hash_str(joint->name);
            hmput(info.name_hash, hash_name, joint_index);
        }
        
        process_joint_info(root_joint,NULL, &info);

        cgltf_animation anim = data->animations[0];


        animation = danim_load(&anim, info);
        animator = danimator_init(NULL, &animation, ibm, 1);
                                                                                        
        
    }

    cgltf_free(data);
    return model;
}


extern dgRT def_rt;
void draw_model(dgDevice *ddev, dModel *m, mat4 model)
{
    DPROFILER_START("model_render");
    dgTexture *texture_slots[DG_MAX_DESCRIPTOR_SET_BINDINGS];
    //TODO: animator SHOULDN't be global, also change it to animation controller
    danimator_animate(&animator);
    animator.model_mat = model;
    dAnimSocket socket = danimator_make_socket(&animator,"mixamorig:LeftHand", m4d(1.0));
    draw_cube(&dd, mat4_mul(danimator_get_socket_transform(&animator, socket), mat4_scale(v3(10,10,10))));

    dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->anim_pipe);
    

    
    for (u32 i = 0; i< m->meshes_count;++i)
    {
        dgBuffer buffers[] = {m->gpu_buf,m->gpu_buf,m->gpu_buf,m->meshes[i].joint_buf, m->gpu_buf};
        for (u32 j = 0; j < m->meshes[i].primitives_count; ++j)
        {
            dMeshPrimitive *p = &m->meshes[i].primitives[j];
            mat4 object_data[MAX_JOINT_COUNT+1] = {model};
            memcpy(&object_data[1], animator.gjm, sizeof(mat4)*animator.anim->skeleton_info.joint_count);
            memcpy(&object_data[MAX_JOINT_COUNT], &p->m.col, sizeof(vec4));
            
            dg_set_desc_set(ddev,&ddev->anim_pipe, object_data, sizeof(object_data), 1);
            texture_slots[0] = &p->m.textures[0];
            texture_slots[1] = &p->m.textures[1];
            texture_slots[2] = &p->m.textures[2];
            texture_slots[3] = &p->m.textures[3];
            dg_set_desc_set(ddev,&ddev->anim_pipe, texture_slots, 4, 2);
            u64 offsets[] = {p->tex_offset.x,p->pos_offset.x,p->norm_offset.x,p->joint_offset.x,p->weight_offset.x};
            dg_bind_vertex_buffers(ddev, buffers, offsets, 5);
            if (p->index_offset.y)
                dg_bind_index_buffer(ddev, &m->gpu_buf, p->index_offset.x);
            
            dg_draw(ddev, p->pos_offset.y/sizeof(vec3),p->index_offset.y/sizeof(u16));
        }
        
    }



    dg_rendering_end(ddev);
    DPROFILER_END();
}

void draw_model_def_shadow(dgDevice *ddev, dModel *m, mat4 model, mat4 *lsms)
{
    DPROFILER_START("model_render");
    if (!ddev->shadow_pass_active)return;
    
    dg_rendering_begin(ddev, &csm_rt.color_attachments[0], 0, &csm_rt.depth_attachment, DG_RENDERING_SETTINGS_MULTIVIEW);
    dg_set_viewport(ddev, 0,0,csm_rt.color_attachments[0].width, csm_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,csm_rt.color_attachments[0].width, csm_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->pbr_shadow_pipe);

    for (u32 i = 0; i< m->meshes_count;++i)
    {
        dgBuffer buffers[] = {m->gpu_buf,m->gpu_buf,m->gpu_buf};
        for (u32 j = 0; j < m->meshes[i].primitives_count; ++j)
        {
            dMeshPrimitive *p = &m->meshes[i].primitives[j];
            u64 offsets[] = {p->pos_offset.x,p->norm_offset.x,p->tex_offset.x};
            dg_bind_vertex_buffers(ddev, buffers, offsets, 3);
            if (p->index_offset.y)
                dg_bind_index_buffer(ddev, &m->gpu_buf, p->index_offset.x);
            
            mat4 object_data[5] = {model};
            memcpy(&object_data[1],lsms,sizeof(mat4)*4);
            //object_data[1]= lsms[cascade_index];
            dg_set_desc_set(ddev,&ddev->pbr_shadow_pipe, object_data, sizeof(object_data), 1);
            dg_draw(ddev, p->pos_offset.y/sizeof(vec3),p->index_offset.y/sizeof(u16));
        }


        
    }
    dg_rendering_end(ddev);
    DPROFILER_END();
}

void draw_model_def(dgDevice *ddev, dModel *m, mat4 model)
{
    DPROFILER_START("model_render");
    dgTexture *texture_slots[DG_MAX_DESCRIPTOR_SET_BINDINGS];
    dg_rendering_begin(ddev, def_rt.color_attachments, 3, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->pbr_def_pipe);
    mat4 object_data[2] = {model, {1.0,1.0,1.0,1.0,1.0,1.0}};
    dg_set_desc_set(ddev,&ddev->pbr_def_pipe, object_data, sizeof(object_data), 1);
    
    for (u32 i = 0; i< m->meshes_count;++i)
    {
        dgBuffer buffers[] = {m->gpu_buf,m->gpu_buf,m->gpu_buf,m->gpu_buf};
        for (u32 j = 0; j < m->meshes[i].primitives_count; ++j)
        {
            dMeshPrimitive *p = &m->meshes[i].primitives[j];
            texture_slots[0] = &p->m.textures[0];
            texture_slots[1] = &p->m.textures[1];
            texture_slots[2] = &p->m.textures[2];
            texture_slots[3] = &p->m.textures[3];
            dg_set_desc_set(ddev,&ddev->pbr_def_pipe, texture_slots, 4, 2);
            u64 offsets[] = {p->tex_offset.x,p->norm_offset.x,p->pos_offset.x,p->tang_offset.x};
            dg_bind_vertex_buffers(ddev, buffers, offsets, 4);
            if (p->index_offset.y)
                dg_bind_index_buffer(ddev, &m->gpu_buf, p->index_offset.x);

            dg_draw(ddev, p->pos_offset.y/sizeof(vec3),p->index_offset.y/sizeof(u16));
        }
        
    }

    dg_rendering_end(ddev);
    DPROFILER_END();

}
