#include "dcore.h"


//this is an application, it should be a thing on its own! (an exe referencing dengine.dll)


//FEATURES TODO
//make the engine self compile (no external dependencies) to a single executable
//shader/texture CACHING!!!!!
//-Fibers (w/ multiple threads)
//-Engine logging
//-Profiling
//-Basic Sound (+ Audio compression/decompression)



int numberr = 122;

void print_number(void *data)
{
    printf("%i\n", numberr);

    djob_request(REQ_EXIT, 0);
}
void print_nonsense(void *data)
{
    printf("%s\n", (char*)data);
    printf("nonsense1\n");
    djob_request(REQ_YIELD, 0);

    printf("nonsense2\n");
    djob_request(REQ_EXIT, 0);
    printf("we should never get here????");
}

void init(void)
{
    dwindow_create(&main_window, "Main Window", 600, 400, DWINDOW_OPT_VULKAN | DWINDOW_OPT_RESIZABLE);
    dcore_init();
}

b32 update(void)
{
    dinput_update();
    if (dkey_pressed(DK_0))printf("DK_%c pressed!\n", '0');
    if (dkey_released(DK_0))printf("DK_%c released!\n", '0');

    if (dkey_pressed(DK_A))printf("DK_%c pressed!\n", 'A');
    if (dkey_released(DK_A))printf("DK_%c released!\n", 'A');

    if (dkey_pressed(DK_LEFT))printf("DK_LEFT pressed!\n");
    if (dkey_released(DK_LEFT))printf("DK_LEFT released!\n");

    if (dkey_pressed(DK_LMB))printf("DK_LMB pressed!\n");
    if (dkey_released(DK_LMB))printf("DK_LMB released!\n");

    return 1;
}

void destroy(void)
{
    dcore_destroy();
}

int main(void)
{
    
    init();
    while(update())
        dcore_update();//update the state of the engine for each step



    dJobDecl job_decl = {print_nonsense, arg00};
    dJobDecl job_decl2 = {print_number, &numberr};

    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_queue_add_job(&job_manager.job_queue, job_decl2);
    djob_queue_add_job(&job_manager.job_queue, job_decl);
    djob_manager_work(&job_manager);



    destroy();
    return 0;
}
