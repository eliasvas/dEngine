#ifndef __DCORE__
#define __DCORE__
#include "tools.h"
//include all subsystems so we can manage them
#include "dtime.h"
#include "dfiber.h"
#include "dthread.h"

void dcore_init(void);

void dcore_update(void);

void dcore_destroy(void);

static Arena temp_arena, main_arena;

#endif