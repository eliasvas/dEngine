#ifndef DFIBER_H
#define DFIBER_H
#include "tools.h"
/*
    This is a simple N:1 fiber implementation,
    later in the engine this should become M:N as in 
    having many threads runnings fibers in parallel!
*/

typedef struct 
{
  void *rip, *rsp;
  void *rbx, *rbp, *r12, *r13, *r14, *r15;//, *rdi, *rsi;
#if defined(BUILD_WIN)
  __m128i xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#endif
}Context;

Context main_context, **task_context;

//these are implemented in assembly and linked afterwards

extern int get_context(Context *c); //fills 'c' with the current context

extern void set_context(Context *c); //sets the current context as 'c'

extern void swap_context(Context *c1,Context *c2); //saves current context in 'c1' and loads the context in 'c2'

b32 fibers_ok(void);

//adds another job to the fiber system
//dfiber_add_job()

//executes ALL jobs dispatched
//dfiber_exec_jobs()

//clears ALL jobs (end of frame??)
//dfiber_clear_jobs()
typedef struct dJobDecl
{
  void (*task)(void *data);
  void *data;
}dJobDecl;

#define JOB_QUEUE_SIZE 1024
#define JOB_QUEUE_STACK_SIZE 2048 
typedef struct dJobQueue
{
  Context jobs[JOB_QUEUE_SIZE];
  void *sp;
  u32 end_index;
  u32 start_index;
}dJobQueue;

static void djob_queue_init(dJobQueue *job_queue)
{
  char *data = malloc(JOB_QUEUE_STACK_SIZE);
  //this points to the end of data, because stack grows downwards
  char *sp = (char*)(data + sizeof(JOB_QUEUE_STACK_SIZE)); 
  //align stack pointer to 16 byte boundary
  sp = (char*)((uintptr_t)sp & -16L);
  //make 128 byte scratch space for the Red Zone
  sp -= 128;
  job_queue->sp = sp;

  job_queue->start_index = 0;
  job_queue->end_index = 0;
}

static void djob_queue_add_job(dJobQueue *job_queue, dJobDecl job)
{
  //if (job_queue->current_index >= JOB_QUEUE_SIZE)
  Context c = {0};
  c.rip = job.task;
  c.rsp = job_queue->sp; 
  job_queue->jobs[job_queue->end_index++ % JOB_QUEUE_SIZE] = c;
}

static Context *djob_queue_remove_job(dJobQueue *job_queue)
{
  if (job_queue->start_index == job_queue->end_index)return NULL; //no jobs 
  Context *ctbr = &job_queue->jobs[job_queue->start_index % JOB_QUEUE_SIZE];
  //delete any state in the context we don't want (right now there is none)
  job_queue->start_index++;

  return ctbr; //we return the start_index job
}


typedef u64 AtomicCounter;
typedef struct dJobManager
{
  dJobQueue job_queue;
  AtomicCounter next_index;
}dJobManager;

dJobManager job_manager;

typedef enum dJobRequest
{
  REQ_YIELD = 1,
  REQ_EXIT,
  REQ_SEND,
  REQ_RECV,
  REQ_NUM
}dJobRequest;

dJobRequest global_request;
u32 global_arg;

//task ---request---> main
b32 djob_request(dJobRequest req, u32 arg);

//main ---handle---> task 
b32 djob_handle(dJobRequest req,u32 arg);

void djob_manager_work(dJobManager *m);

void djob_manager_init(dJobManager *m);

#endif