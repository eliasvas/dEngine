#ifndef __DCORE__
#define __DCORE__
#include "tools.h"
#include "microui/microui.h"
//include all subsystems so we can manage them
#include "dmem.h"
#include "dconfig.h"
#include "dtime.h"
#include "dthread.h"
#include "dinput.h"
#include "dwin.h"
#include "dgfx.h"
#include "dprofiler.h"
#include "dlog.h"
#include "stb/stb_ds.h"
#include "dentity.h"
//#include "dfiber.h"


void dcore_init(void);

void dcore_update(void);

void dcore_destroy(void);

//static Arena temp_arena, main_arena;
static dLinearAllocator temp_alloc, scratch_alloc;  

#endif