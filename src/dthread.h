#ifndef __DTHREAD__ 
#define __DTHREAD__ 

#include "tools.h"
typedef struct DThread
{
#ifdef BUILD_WIN
    HANDLE handle;
    u32 id;
#else
#endif
}DThread;



typedef struct DMutex
{
#ifdef BUILD_WIN
   HANDLE handle; 
#else
#endif
}DMutex;

DThread dthread_create(void *proc, void *params);
void dthread_wait_end(DThread *t, u32 millis);
DMutex dmutex_create(void);
u32 dmutex_lock(DMutex *m);
u32 dmutex_unlock(DMutex *m);

/* NOTE(inv): Basic threading test code

DMutex foo_mutex;
void foo(void *i) //simply increments a value
{
    dmutex_lock(&foo_mutex);
    (*((u32*)i))++;
    dmutex_unlock(&foo_mutex);
}
..
..
    u32 i = 4;
    foo_mutex= dmutex_create();
    DThread t = dthread_create(foo, &i);
    DThread t2 = dthread_create(foo, &i);

    printf("i before thread increment: %i\n", i);
    dthread_wait_end(&t, INFINITE_MS);
    dthread_wait_end(&t2, INFINITE_MS);
    printf("i after thread increment: %i\n", i);
    assert(i == 6);
*/

#endif