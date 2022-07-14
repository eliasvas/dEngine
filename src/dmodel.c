#include "dmodel.h"
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf/cgltf.h"
#include "dlog.h"


extern dgDevice dd;
mat4 gjoint_matrices[25];

//joint index is the last number in the joint's name
u32 extract_joint_index(char *joint_name)
{
    u32 num = atoi(strrchr(joint_name, '_') + sizeof(char));
    return num;
}

dJointInfo *process_joint_info(cgltf_node *joint, dJointInfo *parent, dSkeletonInfo *info)
{
    if (joint == NULL)return;
    u32 joint_index = extract_joint_index(joint->name);
    hmput(info->name_hash, hash_str(joint->name), joint_index);
    info->joint_count++;
    dJointInfo *j = &info->joint_hierarchy[joint_index];
    j->parent = parent;
    j->id = joint_index;
    j->children_count = 0;
    for (u32 i = 0; i < joint->children_count; ++i){
        j->children[j->children_count++] = process_joint_info(joint->children[i],j, info);
    }
    return j;
}

dModel dmodel_load_gltf(const char *filename)
{
    char filepath[256];
    sprintf(filepath,"../assets/%s/%s.gltf",filename,filename);
    dlog(NULL, "gltf FILEPATH: %s\n", filepath);
    dModel model = {0};

    cgltf_options options = {0};
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
    for (u32 i = 0; i< data->textures_count;++i)
    {
        sprintf(filepath, "../assets/%s/%s", filename,data->textures[i].image->uri);
        dlog(NULL, "image FILEPATH: %s\n", filepath);

        //FIX: VERY important for normal mapping, all non opaque/diffuse textures should be linear!
        if (strstr(data->textures[i].image->uri, "ase") != NULL)
            model.textures[DMODEL_BASE_COLOR_INDEX] = dg_create_texture_image(&dd,filepath,VK_FORMAT_R8G8B8A8_SRGB);
        else if (strstr(data->textures[i].image->uri, "etallic") != NULL)
            model.textures[DMODEL_ORM_INDEX] = dg_create_texture_image(&dd,filepath,VK_FORMAT_R8G8B8A8_SRGB);
        else if (strstr(data->textures[i].image->uri, "ormal") != NULL)
            model.textures[DMODEL_NORMAL_INDEX] = dg_create_texture_image(&dd,filepath,VK_FORMAT_R8G8B8A8_SRGB);
        else if (strstr(data->textures[i].image->uri, "missive") != NULL)
            model.textures[DMODEL_EMISSIVE_INDEX] = dg_create_texture_image(&dd,filepath,VK_FORMAT_R8G8B8A8_SRGB);
    } 
    //fill the rest of the textures that couldnt be loaded with NULL
    for (u32 i = 0; i < 4; ++i)
    {
        //if there isn't a texture loaded, load black
        if (model.textures[i].width == 0)
            model.textures[i] = dg_create_texture_image_wdata(&dd,NULL, 64,64, VK_FORMAT_R8G8B8A8_SRGB);
    }

    for (u32 i = 0; i< data->meshes_count; ++i)
    {
        dMesh mesh = {0};
        //FIX: we only support one primitive (e.g) triangle per mesh
        cgltf_primitive primitive = data->meshes[i].primitives[0];
        cgltf_float *weights = data->meshes[i].weights;

        s32 norm_index = -1;
        s32 pos_index = -1;
        s32 tex_index = -1;
        s32 joint_index = -1;
        s32 weight_index = -1;
        s32 tangent_index = -1;
        for (u32 i = 0; i < primitive.attributes_count; ++i)
        {
            if (strncasecmp("TEX", primitive.attributes[i].name,3) == 0)
                tex_index = i;
            else if (strncasecmp("NORM", primitive.attributes[i].name,4) == 0)
                norm_index = i;
            else if (strncasecmp("POS", primitive.attributes[i].name,3) == 0)
                pos_index = i;
            else if (strncasecmp("TAN", primitive.attributes[i].name,3) == 0)
                tangent_index = i;
            else if (strncasecmp("JOI", primitive.attributes[i].name,3) == 0)
                joint_index = i;
            else if (strncasecmp("WEI", primitive.attributes[i].name,3) == 0)
                weight_index = i;
        }

/*
        //process bones and make a vertex -> bone relation
        if (data->skins_count) {
            u32 vertex_count = primitive.attributes[pos_index].data->count; //1728
            //cgltf_attribute p = primitive.attributes[joint_index];
            cgltf_attribute p = primitive.attributes[weight_index];
            //cgltf_attribute p2 = primitive.accessors[weight_index];
            cgltf_type t =  p.data->component_type;
            for (u32 i = 0; i < vertex_count; ++i){
                printf("Vertex: %i %s\n", i, p.data[i].buffer_view->name);
            }
            cgltf_skin skin = data->skins[0];
        }
        */
       

        //create pos buffer 
        if (pos_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.pos_buf,primitive.attributes[pos_index].data->buffer_view->size,(char*)primitive.attributes[pos_index].data->buffer_view->buffer->data + primitive.attributes[pos_index].data->buffer_view->offset);
    
        //create tex buffer 
        if (tex_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.tex_buf,primitive.attributes[tex_index].data->buffer_view->size,(char*)primitive.attributes[tex_index].data->buffer_view->buffer->data + primitive.attributes[tex_index].data->buffer_view->offset);

        //create norm buffer 
        if (norm_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.norm_buf,primitive.attributes[norm_index].data->buffer_view->size,(char*)primitive.attributes[norm_index].data->buffer_view->buffer->data + primitive.attributes[norm_index].data->buffer_view->offset);
 
        //create tangent buffer 
        if (tangent_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.tang_buf,primitive.attributes[tangent_index].data->buffer_view->size,(char*)primitive.attributes[tangent_index].data->buffer_view->buffer->data + primitive.attributes[tangent_index].data->buffer_view->offset);

        //create joint buffer 
        if (joint_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.joint_buf,primitive.attributes[joint_index].data->buffer_view->size,(char*)primitive.attributes[joint_index].data->buffer_view->buffer->data + primitive.attributes[joint_index].data->buffer_view->offset);
 
        
        //create weight buffer 
        if (weight_index != -1)
            dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.weight_buf,primitive.attributes[weight_index].data->buffer_view->size,(char*)primitive.attributes[weight_index].data->buffer_view->buffer->data + primitive.attributes[weight_index].data->buffer_view->offset);
 

        if (joint_index != -1) {
            cgltf_attribute joint_attrib = primitive.attributes[joint_index];
            cgltf_attribute weight_attrib = primitive.attributes[weight_index];
            printf("ok!\n");

        }
        
        if (primitive.indices)
        {
            //create index buffer
            dg_create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
            (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            &mesh.index_buf, primitive.indices->count *sizeof(u16), (char*)primitive.indices->buffer_view->buffer->data + primitive.indices->buffer_view->offset);
        }


        
        dSkeletonInfo info = {0};
        if (data->animations_count){
            cgltf_node *root_joint = data->skins[0].joints[0];
            process_joint_info(root_joint->children[0],NULL, &info);

            cgltf_animation anim = data->animations[2];
            for (u32 i = 0; i < anim.channels_count; ++i)
            {
                cgltf_node *node = anim.channels[i].target_node;

                cgltf_float *t = node->translation;
                cgltf_float *r = node->rotation;
                mat4 translation = mat4_translate(v3(t[0],t[1],t[2]));
                mat4 rotation = quat_to_mat4((Quaternion){r[0],r[1],r[2],r[3]});
                mat4 m = mat4_mul(translation, rotation);
                gjoint_matrices[extract_joint_index(node->name)] = m;
            }
        }

        model.meshes[model.meshes_count++] = mesh;

    }

    

    cgltf_free(data);
    return model;
}


extern dgRT def_rt;
void draw_model(dgDevice *ddev, dModel *m, mat4 model)
{
    dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, FALSE, FALSE);
    dg_set_viewport(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_bind_pipeline(ddev, &ddev->anim_pipe);
    dgBuffer buffers[] = {m->meshes[0].tex_buf,m->meshes[0].pos_buf,m->meshes[0].joint_buf,m->meshes[0].weight_buf};
    u64 offsets[] = {0,0,0,0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 4);
    if (m->meshes[0].index_buf.active)
        dg_bind_index_buffer(ddev, &m->meshes[0].index_buf, 0);


    mat4 object_data[26] = {model};
    mat4 I = m4d(1.f);
    memcpy(&object_data[1], &gjoint_matrices, sizeof(I)*25);
    object_data[1] = I;
    dg_set_desc_set(ddev,&ddev->anim_pipe, object_data, sizeof(object_data), 1);
    dg_set_desc_set(ddev,&ddev->anim_pipe, &m->textures[0], 4, 2);
    dg_draw(ddev, m->meshes[0].pos_buf.size,m->meshes[0].index_buf.size/sizeof(u16));

    dg_rendering_end(ddev);
}


void draw_model_def(dgDevice *ddev, dModel *m, mat4 model)
{

    dg_rendering_begin(ddev, def_rt.color_attachments, 3, &def_rt.depth_attachment, FALSE, FALSE);
    dg_set_viewport(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->pbr_def_pipe);
    //dgBuffer buffers[] = {m->meshes[0].tex_buf,m->meshes[0].norm_buf,m->meshes[0].tang_buf,m->meshes[0].pos_buf};
    dgBuffer buffers[] = {m->meshes[0].tex_buf,m->meshes[0].norm_buf,m->meshes[0].pos_buf};
    u64 offsets[] = {0,0,0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 3);
    if (m->meshes[0].index_buf.active)
        dg_bind_index_buffer(ddev, &m->meshes[0].index_buf, 0);



    mat4 object_data[2] = {model, {1.0,1.0,1.0,1.0,1.0,1.0}};
    dg_set_desc_set(ddev,&ddev->pbr_def_pipe, object_data, sizeof(object_data), 1);
    dg_set_desc_set(ddev,&ddev->pbr_def_pipe, &m->textures[0], 4, 2);
    dg_draw(ddev, m->meshes[0].pos_buf.size,m->meshes[0].index_buf.size/sizeof(u16));


    dg_rendering_end(ddev);

}
