#ifndef __DTHREAD__ 
#define __DTHREAD__ 

#include "tools.h"


#if defined(BUILD_UNIX)
#include <pthread.h>
#endif

#define INFINITE_MS 4294967295

typedef struct dThread
{
#ifdef BUILD_WIN
    HANDLE handle;
    u32 id;
#else
    pthread_t thread;
#endif
}dThread;

b32 dthreads_ok(void);



typedef struct dMutex
{
#if defined(BUILD_WIN)
   HANDLE handle; 
#else
    pthread_mutex_t mutex;
#endif
}dMutex;

dThread dthread_create(void *proc, void *params);
void dthread_wait_end(dThread *t, u32 millis);
dMutex dmutex_create(void);
u32 dmutex_lock(dMutex *m);
u32 dmutex_unlock(dMutex *m);
b32 threads_ok(void); //unit test to see if threading works fine

#endif