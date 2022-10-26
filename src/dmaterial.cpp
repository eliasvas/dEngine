#include "dmaterial.h"

#include "dgfx.h"
extern dgDevice dd;


dTextureManager tex_manager;

static dMaterial dmaterial_basic(void)
{
    dMaterial mat = {0};

    return mat;
}




void dTextureManager::init(void){
    this->texture_hash = NULL;
    this->ref_count.init(16);
    this->textures.init(16);
    hmdefault(this->texture_hash, DTEXTURE_NOT_FOUND);
}
dgTexture* dTextureManager::addTex(char *name, dgImageFormat f){
    u64 name_hash = hash_str(name);
    u32 tex_index = hmget(this->texture_hash, name_hash);
    if (tex_index == DTEXTURE_NOT_FOUND){
        tex_index = this->textures.size();
        this->ref_count.push_back(1);
        this->textures.push_back(dg_create_texture_image(&dd, name, f));
    }else{
        this->ref_count[tex_index]++;
    }
    return &this->textures[tex_index];
}

//TODO: hash_str is EXPENSIVE, we should store intermediate
//results because this manager is used ALOT!
void dTextureManager::delTex(char *name){
    u64 name_hash = hash_str(name);
    u32 tex_index = hmget(this->texture_hash, name_hash);
    if (tex_index == DTEXTURE_NOT_FOUND){
        return;
    }else{
        this->ref_count[tex_index]--;
        if (this->ref_count[tex_index] <= 0){
            //if texture should be destroyed, swap it with the last one
            u64 last_tex_hash = hash_str(this->textures[this->textures.size()-1].name);

            //delete from the hashmap the last tex index and the "to be deleted index"
            hmdel(this->texture_hash, name_hash);
            hmdel(this->texture_hash, last_tex_hash);
            this->textures[tex_index] = this->textures[this->textures.size()-1];
            this->textures.pop_back();
            this->ref_count[tex_index]= this->ref_count[this->textures.size()-1];
            this->ref_count.pop_back();
            hmput(this->texture_hash, tex_index, last_tex_hash);
        }
    }
}

void dTextureManager::deinit(void){}