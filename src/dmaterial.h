#ifndef __DMATERIAL__
#define  __DMATERIAL__
#include "tools.h"
#include "stb/stb_ds.h"
#include "dgfx.h"

typedef enum dMaterialSettings
{
    DMATERIAL_BASE_COLOR= 0x1,
    DMATERIAL_EMISSIVE= 0x2,
    DMATERIAL_NORMAL= 0x4,
    DMATERIAL_ORM= 0x8,
}dMaterialSettings;

//indexes in the texture array?
#define DMATERIAL_BASE_COLOR_INDEX 0
#define DMATERIAL_ORM_INDEX 1
#define DMATERIAL_NORMAL_INDEX 2
#define DMATERIAL_EMISSIVE_INDEX 3


#define DMATERIAL_MAX_TEXTURES 8
struct dMaterial
{
    dgTexture textures[DMATERIAL_MAX_TEXTURES];
    vec4 col;
    dMaterialSettings settings;
};


static dMaterial dmaterial_basic(void);
#define DTEXTURE_MANAGER_MAX_TEXTURES 100
#define DTEXTURE_NOT_FOUND 0xFFFFFFFF

struct dTextureManager{
    dgTexture textures[DTEXTURE_MANAGER_MAX_TEXTURES]; //TODO: this should be completely dynamic
    u32 ref_count[DTEXTURE_MANAGER_MAX_TEXTURES];
    u32 textures_count;

    struct {u64 key; u32 value;}*texture_hash;//maps texture's name hash to index in textures array

    void init();
    dgTexture * addTex(char *name, dgImageFormat f);
    void delTex(char *name);
    void deinit();
};

void dtexture_manager_init(dTextureManager *tm);
dgTexture* dtexture_manager_add_tex(dTextureManager *tm, char *name, dgImageFormat f);
void dtexture_manager_del_tex(dTextureManager *tm, char *name);

#endif