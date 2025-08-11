#ifndef PLUG_H
#define PLUG_H

#include <raylib.h>
#include <complex.h>
#include "dsa.h"

DECLARE_DA(const char*, String)

#define N 256
#define MAX 200

typedef struct {
    float left;
    float right;
} Frame;

typedef struct {
    float in[N];
    float complex out[N];
    float max_amp;
    Music music[MAX];
} Plug;

typedef void (*plug_init_t)(Plug *plug, String_DA *music_path, int *file_counter);
typedef void (*plug_update_t)(Plug *plug, String_DA *music_path, int *file_counter, int *item, bool *requested);

#endif // PLUG_H

