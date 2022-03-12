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
    swap_context(*(task_context), &main_context);
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

void djob_manager_work(dJobManager *m)
{
    printf("djob_manager main start\n");
    volatile b32 exit = FALSE;
    while(!exit)
    {
        if (job_manager.job_queue.start_index == job_manager.job_queue.end_index)break;
        Context *current_task = djob_queue_remove_job(&m->job_queue);
        //@investigate
        task_context = &current_task;
        swap_context(&main_context, *task_context);
        djob_handle(global_request, global_arg);
    }
    printf("djob_manager finished all jobs");
}

void djob_manager_init(dJobManager *m)
{
    djob_queue_init(&m->job_queue);
    m->next_index = 0;
}
