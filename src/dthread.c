
#include "dthread.h"

DThread dthread_create(void *proc, void *params)
{
    DThread t;
    memset(&t, 0, sizeof(t));
#if  defined(BUILD_WIN)
    t.handle = CreateThread(0,0,proc, params, 0, &t.id);
#else
    s32 terr = pthread_create(&t.thread, NULL, proc, params);
#endif
    return t;
}

void dthread_wait_end(DThread *t, u32 millis)
{
#if defined(BUILD_WIN)
    if (millis == INFINITE_MS)millis = INFINITE;
    WaitForSingleObject(t->handle, millis);
#else
    pthread_join(t->thread, NULL);
#endif
}

DMutex dmutex_create(void)
{
    DMutex m;
#if defined(BUILD_WIN)
    m.handle = CreateMutexA(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&m.mutex, NULL);
#endif
    return m;
}

u32 dmutex_lock(DMutex *m)
{

#if defined(BUILD_WIN) 
    WaitForSingleObject(m->handle, INFINITE);
    return TRUE;
#else
    pthread_mutex_lock(&m->mutex);
#endif
    return FALSE;
}

u32 dmutex_unlock(DMutex *m)
{
#if defined(BUILD_WIN)
    ReleaseMutex(m->handle);
    return TRUE;
#else
    pthread_mutex_unlock(&m->mutex);
#endif
    
    return FALSE;
}

static DMutex foo_mutex;
void thread_inc(void *i) //simply increments a value
{
    dmutex_lock(&foo_mutex);
    (*((u32*)i))++;
    dmutex_unlock(&foo_mutex);
}
b32 threads_ok(void)
{
    u32 i = 4;
    foo_mutex= dmutex_create();
    DThread t1 = dthread_create(thread_inc, &i);
    DThread t2 = dthread_create(thread_inc, &i);

    //printf("i before thread increment: %i\n", i);
    dthread_wait_end(&t1, INFINITE_MS);
    dthread_wait_end(&t2, INFINITE_MS);
    //printf("i after thread increment: %i\n", i);
    return (i==6);
}

