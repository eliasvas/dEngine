#ifndef __DCORE__
#define __DCORE__
#define VK_NO_PROTOTYPES
#include "tools.h"
//include all subsystems so we can manage them
#include "dmem.h"
#include "deditor.h"
#include "dconfig.h"
#include "dtime.h"
#include "dthread.h"
#include "dinput.h"
#include "dwin.h"
#include "dgfx.h"
#include "dprofiler.h"
#include "dlog.h"
#include "darray.h"

#include "stb/stb_ds.h"
#include "dentity.h"
#include "dmaterial.h"
#include "dcamera.h"
#include "dparticle.h"

//#include "dfiber.h"


void dcore_init(void);

void dcore_update(f64 dt);

void dcore_destroy(void);


#endif