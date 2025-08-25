#ifndef HOT_RELOAD
#define HOT_RELOAD

#include <stdbool.h>
#include "plug.h"

extern plug_update_t plug_update;
extern plug_init_t plug_init;

bool reload_libplug(void);

#endif //HOT_RELOAD
