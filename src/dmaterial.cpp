#include "dmaterial.h"

#include "dgfx.h"
extern dgDevice dd;


dTextureManager tex_manager;

static dMaterial dmaterial_basic(void)
{
    dMaterial mat = {0};

    return mat;
}


typedef dHandle dTextureHandle;

void dTextureManager::init(void){
    this->texture_hash = NULL;
    this->ref_count.init(16);
    this->textures.init(16);
    hmdefault(this->texture_hash, DTEXTURE_NOT_FOUND);
    this->tex_handles.init();
}


dgTexture* dTextureManager::addTex(char *name, dgImageFormat f){
    u64 name_hash = hash_str(name);
    dTextureHandle tex_handle;
    tex_handle.id = hmget(this->texture_hash, name_hash);

    if (tex_handle.id == DTEXTURE_NOT_FOUND){
        tex_handle = this->tex_handles.create();
        this->ref_count.resize(tex_handle.index());
        this->textures.resize(tex_handle.index());

        this->ref_count[tex_handle.index()] = 1;
        this->textures[tex_handle.index()] = dg_create_texture_image(&dd, name, f);
    }else{
        this->ref_count[tex_handle.index()]++;
    }
    return &this->textures[tex_handle.index()];
}

//TODO: hash_str is EXPENSIVE, we should store intermediate
//results because this manager is used ALOT!
void dTextureManager::delTex(char *name){
    u64 name_hash = hash_str(name);
    dTextureHandle tex_handle;
    tex_handle.id = hmget(this->texture_hash, name_hash);

    if (tex_handle.id == DTEXTURE_NOT_FOUND){
        return;
    }else{
        this->ref_count[tex_handle.index()]--;
        if (this->ref_count[tex_handle.index()] <= 0){
            hmdel(this->texture_hash, name_hash);
            dg_cleanup_texture(&dd, &this->textures[tex_handle.index()]);
        }
    }
}

void dTextureManager::deinit(void){
    this->ref_count.deinit();
    this->tex_handles.deinit();
    this->textures.deinit();
    hmfree(this->texture_hash);
}