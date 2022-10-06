#ifndef __DMODEL__
#define __DMODEL__
#include "dgfx.h"
#include "dmaterial.h"
#include "stb/stb_ds.h"
#include "danim.h"
#include "darray.h"
#define DMODEL_MAX_TEXTURES 8

struct dMeshPrimitive{
    //offsets encode the [start, finish] within the 
    //component's buffer where the vertex data are for the primitive
    uvec2 pos_offset;
    uvec2 norm_offset;
    uvec2 tex_offset;
    uvec2 tang_offset;
    uvec2 col_offset;
    uvec2 joint_offset;
    uvec2 weight_offset;

    uvec2 index_offset;

    dMaterial m;

    u32 primitive_type;
};

struct dMesh {
    dgBuffer joint_buf; //GPU buffer storing per-vertex joint IDs
    

    dArray<dMeshPrimitive> primitives;
    dSkeletonInfo skeleton_info;
};


struct dModel {
    //dgBuffer vertex_buffer;
    dgBuffer gpu_buf;
    dArray<dMesh> meshes;

    dgTexture textures[DMODEL_MAX_TEXTURES];
    u32 textures_count;

    b32 finished_loading; //maybe this should be a thing in the asset pipeline
    dMaterial material;
};
void dmodel_load_gltf(const char *filename, dModel *m);



void draw_model_def_shadow(dgDevice *ddev, dModel *m, mat4 model, mat4 *lsms);
void draw_model_def(dgDevice *ddev, dModel *m, mat4 model);
void draw_model(dgDevice *ddev, dModel *m, mat4 model);





struct dModelHandle {
    u32 handle;
};
#define DMODEL_MAX_FREE_INDICES 2048
struct dModelManager {
    dMallocAllocator *allocator;
    dArray<u8> generation; //tracks the generation of its index, via generation[index]
    u32 *free_indices[DMODEL_MAX_FREE_INDICES]; //holds indices that have been deleated
    u32 indices_start_index, indices_end_index;

    dArray<dModel> models; // models[i] is the model at index i

    void init(void);
    void deinit(void);
    //DOC: creates a unique handle for a model (or adds a reference count if it exists)
    dModelHandle create_handle(void);
    b32 alive(dModelHandle h);
};

dModelHandle dmodel_handle_make(u32 index, u8 generation);

#endif