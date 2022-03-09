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
  void *rbx, *rbp, *r12, *r13, *r14, *r15, *rdi, *rsi;
#if defined(BUILD_WIN)
  __m128i xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#endif
}Context;

//these are implemented in assembly and linked afterwards
extern int get_context(Context *c);
extern void set_context(Context *c);
extern void swap_context(Context *c);

//adds another job to the fiber system
//dfiber_add_job()

//executes ALL jobs dispatched
//dfiber_exec_jobs()

//clears ALL jobs (end of frame??)
//dfiber_clear_jobs()
#endif