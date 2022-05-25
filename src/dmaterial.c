#include "dmaterial.h"

static dMaterial dmaterial_basic(void)
{
    dMaterial mat = {0};
    mat.base_color  = v3(0.5f,0.4f,0.3f);
    mat.metallic = 0.0f;
    mat.roughness = 0.0f;
}