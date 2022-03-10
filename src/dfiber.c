#include "dfiber.h"

b32 fibers_ok(void)
{
    volatile int x = 0;
    volatile int y = 0;
    Context c = {0};    
    get_context(&c);
    y++;
    if (x == 0)
    {
        x++;
        set_context(&c);
    }
    return (y == 2);
}