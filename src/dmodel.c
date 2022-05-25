#include "dmodel.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

extern dgDevice dd;

dModel dmodel_load_gltf(const char *filename)
{
    char filepath[256];
    sprintf(filepath,"../assets/%s/%s.gltf",filename,filename);
    printf("gltf FILEPATH: %s\n", filepath);
    dModel model = {0};

    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);
    if (result != cgltf_result_success)
    {
        model.finished_loading = 0;
        return model;
    }
    printf("Attr[0] = %s\n",data->meshes[0].primitives[0].attributes[0].name);

    //first load all the textures!
    model.textures_count= data->textures_count;
    for (u32 i = 0; i< data->textures_count;++i)
    {
        sprintf(filepath, "../assets/%s/%s", filename,data->textures[i].image->uri);
        printf("image FILEPATH: %s\n", filepath);
        model.textures[i] = dg_create_texture_image(&dd,filepath,VK_FORMAT_R8G8B8A8_SRGB);
    } 

    for (u32 i = 0; i< data->meshes_count; ++i)
    {

    }


    cgltf_free(data);
    return model;
}