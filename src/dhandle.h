#ifndef DHANDLE_H
#define DHANDLE_H

#include "tools.h"
#include "assert.h"
#include "dmem.h"
#include "dthread.h"
#include "dqueue.h"

//TODO: make the bits arbitrary for generation and index (max 32 bits) (via Template??????)


struct dHandle{
    u32 id;
    u32 index();
    u32 generation();
};
dHandle dhandle_make(u32 index, u8 generation);

struct dHandleManager{
    dArray<u8> generation;
    dQueue<u32> free_indices;


    //DOC: initializes everything
    void init();
    //DOC: frees all allocations
    void deinit();
    //DOC: creates a new handle and returns it
    dHandle create(void);
    //DOC: checks if a handle is alive or not (meaning its being manager OR destroyed)
    b32 alive(dHandle h); //maybe active?
};



#endif