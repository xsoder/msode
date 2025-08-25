#ifndef PLUG_H
#define PLUG_H

#include <raylib.h>
#include <complex.h>
#include "dsa.h"
#include "quickui.h"

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

typedef struct {
    String_DA *music_path;
    int file_counter;
    Texture2D texture;
    Font font;
    int item;
    bool requested;
} Ui;

typedef void (*plug_init_t)(Plug *plug, Ui *ui, qui_Context *ctx);
typedef void (*plug_update_t)(Plug *plug, Ui *ui, qui_Context *ctx);

#endif // PLUG_H
