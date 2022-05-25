#ifndef __DMATERIAL__
#define  __DMATERIAL__
#include "tools.h"
#include "dgfx.h"

typedef enum dMaterialSettings
{
    DMATERIAL_DIFFUSE= 0x1,
    DMATERIAL_SPECULAR= 0x2,
}dMaterialSettings;

typedef struct dMaterial
{
    vec3 base_color;
    f32  metallic;
    f32 roughness;

    dgTexture diffuse;
    dgTexture specular;

    dMaterialSettings settings;
}dMaterial;


static dMaterial dmaterial_basic(void);

#endif