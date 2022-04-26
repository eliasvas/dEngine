#ifndef __DCORE__
#define __DCORE__
#include "tools.h"
#include "microui/microui.h"
//include all subsystems so we can manage them
#include "dconfig.h"
#include "dtime.h"
//#include "dfiber.h"
#include "dthread.h"
#include "dinput.h"
#include "dwin.h"
#include "dgfx.h"

dWindow main_window;

void dcore_init(void);

void dcore_update(void);

void dcore_destroy(void);

static Arena temp_arena, main_arena;

#endif