#ifndef __DMATERIAL__
#define  __DMATERIAL__
#include "tools.h"
#include "dgfx.h"

typedef enum dMaterialSettings
{
    DMATERIAL_BASE_COLOR= 0x1,
    DMATERIAL_EMISSIVE= 0x2,
    DMATERIAL_NORMAL= 0x4,
    DMATERIAL_ORM= 0x8,
}dMaterialSettings;

typedef struct dMaterial
{
    dgTexture base_color;
    dgTexture emissive;
    dgTexture normal;
    dgTexture orm; //occlusion/roughness/metallic

    dMaterialSettings settings;
}dMaterial;


static dMaterial dmaterial_basic(void);

#endif