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
    this->textures_count = 0;
    this->texture_hash = NULL;
    memset(this->ref_count, 0, sizeof(this->ref_count));
    hmdefault(this->texture_hash, DTEXTURE_NOT_FOUND);
}
dgTexture* dTextureManager::addTex(char *name, dgImageFormat f){
    u64 name_hash = hash_str(name);
    u32 tex_index = hmget(this->texture_hash, name_hash);
    if (tex_index == DTEXTURE_NOT_FOUND){
        dgTexture t = dg_create_texture_image(&dd, name, f);
        tex_index = this->textures_count++;
        this->ref_count[tex_index] =1;
        this->textures[tex_index] = t;
        hmput(this->texture_hash, tex_index, name_hash);
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
            u64 last_tex_hash = hash_str(this->textures[this->textures_count].name);

            //delete from the hashmap the last tex index and the "to be deleted index"
            hmdel(this->texture_hash, name_hash);
            hmdel(this->texture_hash, last_tex_hash);
            this->textures[tex_index] = this->textures[this->textures_count];
            this->ref_count[tex_index]= this->ref_count[this->textures_count];
            hmput(this->texture_hash, tex_index, last_tex_hash);
        }
    }
}

void dTextureManager::deinit(void){}