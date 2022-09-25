#include "dmaterial.h"

static dMaterial dmaterial_basic(void)
{
    dMaterial mat = {0};

    return mat;
}



dTextureManager texture_manager;

void dtexture_manager_init(dTextureManager *tm){
    if (tm == NULL)tm = &texture_manager;
    tm->textures_count = 0;
    tm->texture_hash = NULL;
    memset(tm->ref_count, 0, sizeof(tm->ref_count));
    hmdefault(tm->texture_hash, DTEXTURE_NOT_FOUND);
}
#include "dgfx.h"
extern dgDevice dd;
dgTexture* dtexture_manager_add_tex(dTextureManager *tm, char *name, dgImageFormat f){
    if (tm == NULL)tm = &texture_manager;
    u64 name_hash = hash_str(name);
    u32 tex_index = hmget(tm->texture_hash, name_hash);
    if (tex_index == DTEXTURE_NOT_FOUND){
        dgTexture t = dg_create_texture_image(&dd, name, f);
        tex_index = tm->textures_count++;
        tm->ref_count[tex_index] =1;
        tm->textures[tex_index] = t;
        hmput(tm->texture_hash, tex_index, name_hash);
    }else{
        tm->ref_count[tex_index]++;
    }
    return &tm->textures[tex_index];
}

//TODO: hash_str is EXPENSIVE, we should store intermediate
//results because this manager is used ALOT!
void dtexture_manager_del_tex(dTextureManager *tm, char *name){
    if (tm == NULL)tm = &texture_manager;
    u64 name_hash = hash_str(name);
    u32 tex_index = hmget(tm->texture_hash, name_hash);
    if (tex_index == DTEXTURE_NOT_FOUND){
        return;
    }else{
        tm->ref_count[tex_index]--;
        if (tm->ref_count[tex_index] <= 0){
            //if texture should be destroyed, swap it with the last one
            hmdel(tm->texture_hash, name_hash);
            u64 tex_hash = hash_str(tm->textures[tm->textures_count].name);

            hmdel(tm->texture_hash, name_hash);
            tm->textures[tex_index] = tm->textures[tm->textures_count];
            tm->ref_count[tex_index]= tm->ref_count[tm->textures_count];
            hmput(tm->texture_hash, tex_index, tex_hash);
        }
    }
}