#include "hotreload.h"
#include <stdio.h>

plug_update_t plug_update = NULL;
plug_init_t plug_init = NULL;

bool reload_libplug(void)
{
    #if ENABLE_HOT_RELOAD
    #include <dlfcn.h>
        if (libplug != NULL) dlclose(libplug);
    libplug = dlopen(lib_name, RTLD_NOW);
        if (libplug == NULL) {
            fprintf(stderr, "Could not load %s because of this %s\n", lib_name, dlerror());
            return false;
        }
        
        plug_init = dlsym(libplug, "plug_init");
        if (plug_init == NULL) {
            fprintf(stderr, "Could not find plug_init in library %s error: %s\n", lib_name, dlerror());
            return false;
        }
            
        plug_update = dlsym(libplug, "plug_update");
        if (plug_update == NULL) {
            fprintf(stderr, "Could not find plug_update in library %s error: %s\n", lib_name, dlerror());
            return false;
        }
        return true;
    #else
        plug_init = plug_init_imp;
        plug_update = plug_update_imp;
        return true;
    #endif
}

