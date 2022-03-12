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

Context main_context, task_context;

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

typedef u64 AtomicCounter;

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

void djob_manager_main(void);

#endif