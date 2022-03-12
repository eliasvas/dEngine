#include "dfiber.h"

b32 fibers_ok(void)
{
    volatile int x = 0;
    volatile int y = 0;
    Context c = {0};    
    get_context(&c);
    y++;
    if (x == 0)
    {
        x++;
        set_context(&c);
    }
    return (y == 2);
}

//A task/job wants to either yield or exit, thus we 
//put the rquest and arguments on the global variables
//and switch to main to handle them
b32 djob_request(dJobRequest req, u32 arg)
{
    //printf("djob_request\n");
    if (req == REQ_EXIT)global_request = REQ_EXIT;
    swap_context(&task_context, &main_context);
    return 0;
}

//A task/job has issued a request (normally yield or exit)
//and here we handle it, in case of yield we put the job in the back of the job queue
//in the case of exit we just delete the current job and we are done!
b32 djob_handle(dJobRequest req,u32 arg)
{

    //printf("djob_handle\n");
    //if (req == REQ_YIELD)push_to_back_of_queue();
    if (req == REQ_EXIT)return TRUE;
    return FALSE;
}

void djob_manager_main(void)
{
    printf("djob_manager main start\n");
    volatile b32 exit = FALSE;
    while(!exit)
    {
        swap_context(&main_context, &task_context);
        exit = djob_handle(global_request, global_arg);
    }
    printf("djob_manager finished all jobs");
}