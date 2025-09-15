#ifndef PLUG_H
#define PLUG_H

#include "raylib.h"

#include <complex.h>
#include "dsa.h"
#include "quickui.h"

DECLARE_DA(const char*, String)

#define PLUG_BUF_N 8192
#define MAX 200


typedef struct {
    float volume;
    bool fullscreen;
} Config;

typedef struct {
    float left;
    float right;
} Frame;

typedef struct {
    float in[PLUG_BUF_N];
    float complex out[PLUG_BUF_N];
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
    Texture2D play_texture;
    Texture2D pause_texture;
    Texture2D fullscreen_texture;
    Texture2D seek_forward_texture;
    Texture2D seek_backward_texture;
    Texture2D next_texture;
    Texture2D previous_texture;
    Texture2D loop_texture;
} Ui;

typedef void (*plug_init_t)(Plug *plug, Ui *ui, qui_Context *ctx);
typedef void (*plug_update_t)(Plug *plug, Ui *ui, qui_Context *ctx);

void plug_init_imp(Plug *plug, Ui *ui, qui_Context *ctx);
void plug_update_imp(Plug *plug, Ui *ui, qui_Context *ctx);

Texture2D ImageToTexture(const char* path);

#endif // PLUG_H
