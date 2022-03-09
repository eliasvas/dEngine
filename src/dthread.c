
#include "dthread.h"
#define INFINITE_MS 4294967295

DThread dthread_create(void *proc, void *params)
{
    DThread t;
    memset(&t, 0, sizeof(t));
#ifdef BUILD_WIN
    t.handle = CreateThread(0,0,proc, params, 0, &t.id);
#else
    //TBD
#endif
    return t;
}

void dthread_wait_end(DThread *t, u32 millis)
{
#ifdef BUILD_WIN 
    if (millis == INFINITE_MS)millis = INFINITE;
    WaitForSingleObject(t->handle, millis);
#else
    //TBD
#endif
}

DMutex dmutex_create(void)
{
    DMutex m;
#ifdef BUILD_WIN 
    m.handle = CreateMutexA(NULL, FALSE, NULL);
#else
    //TBD
#endif
    return m;
}

u32 dmutex_lock(DMutex *m)
{

#ifdef BUILD_WIN 
    WaitForSingleObject(m->handle, INFINITE);
    return TRUE;
#else
    //TBD
#endif
    return FALSE;
}

u32 dmutex_unlock(DMutex *m)
{
#ifdef BUILD_WIN 
    ReleaseMutex(m->handle);
    return TRUE;
#else
    //TBD
#endif
    
    return FALSE;
}

