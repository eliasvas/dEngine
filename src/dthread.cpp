
#include "dthread.h"

dThread dthread_create(void *proc, void *params)
{
    dThread t;
    memset(&t, 0, sizeof(t));
#if  defined(BUILD_WIN)
    t.handle = CreateThread(0,0,proc, params, 0, &t.id);
#else
    s32 terr = pthread_create(&t.thread, NULL, (void* (*)(void*))proc, params);
#endif
    return t;
}

void dthread_wait_end(dThread *t, u32 millis)
{
#if defined(BUILD_WIN)
    if (millis == INFINITE_MS)millis = INFINITE;
    WaitForSingleObject(t->handle, millis);
#else
    pthread_join(t->thread, NULL);
#endif
}

dMutex dmutex_create(void)
{
    dMutex m;
#if defined(BUILD_WIN)
    m.handle = CreateMutexA(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&m.mutex, NULL);
#endif
    return m;
}

u32 dmutex_lock(dMutex *m)
{

#if defined(BUILD_WIN) 
    WaitForSingleObject(m->handle, INFINITE);
    return TRUE;
#else
    pthread_mutex_lock(&m->mutex);
#endif
    return FALSE;
}

u32 dmutex_unlock(dMutex *m)
{
#if defined(BUILD_WIN)
    ReleaseMutex(m->handle);
    return TRUE;
#else
    pthread_mutex_unlock(&m->mutex);
#endif
    
    return FALSE;
}

static dMutex foo_mutex;
void dthread_inc(void *i) //simply increments a value
{
    dmutex_lock(&foo_mutex);
    (*((u32*)i))++;
    dmutex_unlock(&foo_mutex);
}
b32 dthreads_ok(void)
{
    u32 i = 4;
    foo_mutex= dmutex_create();
    dThread t1 = dthread_create((void*)dthread_inc, &i);
    dThread t2 = dthread_create((void*)dthread_inc, &i);

    //printf("i before thread increment: %i\n", i);
    dthread_wait_end(&t1, INFINITE_MS);
    dthread_wait_end(&t2, INFINITE_MS);
    //printf("i after thread increment: %i\n", i);
    return (i==6);
}

