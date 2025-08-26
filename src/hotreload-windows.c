#include "hotreload.h"
#include <stdio.h>

plug_update_t plug_update = NULL;
plug_init_t plug_init = NULL;

bool reload_libplug(void)
{
    #if ENABLE_HOT_RELOAD
    // TODO
    #else
    plug_init = plug_init_imp;
    plug_update = plug_update_imp;
    return true;
    #endif
}
