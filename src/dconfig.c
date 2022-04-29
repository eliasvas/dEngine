#include "dconfig.h"
#include "stdio.h"

dConfig engine_config;

void dconfig_load(void)
{
    char set[64], field[64];
    memset(&engine_config, 0, sizeof(engine_config));
    FILE *config = fopen("../dconf", "rw");
    if (config == NULL){dconfig_default();return;}
    while(!feof(config))
    {
        fscanf(config,"%s %s", set, field);
        if (strcmp("dgAPI", field) == 0)
        {
            fscanf(config,"%s\n", set);
            if (strcmp("DG_VULKAN", set) == 0)
            {
                engine_config.graphics_api = DG_VULKAN;
            }else if(strcmp("DG_OPENGL", set) == 0)
            {
                engine_config.graphics_api = DG_OPENGL;
            }
            else
            {
                engine_config.graphics_api = DG_SOFTWARE;
            }

        }else if(strcmp("default_resolution", field) == 0)
        {
            fscanf(config, "%f %f\n", &engine_config.default_resolution.x, &engine_config.default_resolution.y);
        }
        else if (strcmp("shader_path", field) == 0)
        {
            fscanf(config, "%s\n", engine_config.shader_path);
        }
        else if (strcmp("spirv_path", field) == 0)
        {
            fscanf(config, "%s\n", engine_config.spirv_path);
        }
    }
    fclose(config);
}

void dconfig_save(void)
{
    FILE* config = fopen("../dconf", "w");
    //TODO(ilias): log into console that action couldn't be completed!
    if (config == NULL)return; 
    fprintf(config, "set dgAPI DG_VULKAN\n");
    fprintf(config, "set spirv_path %s\n", engine_config.spirv_path);
    fprintf(config, "set shader_path %s\n", engine_config.shader_path);
    fprintf(config, "set default_resolution %i %i\n", (int)engine_config.default_resolution.x, (int)engine_config.default_resolution.y);
    fclose(config);
}

void dconfig_default(void)
{
    engine_config.graphics_api = DG_VULKAN;
    engine_config.default_resolution = v2(800,600); 
    sprintf(engine_config.shader_path, "../assets/shaders/");
    sprintf(engine_config.spirv_path, "../build/shaders/");
}