#include <dlfcn.h>
#include "plug.h"
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <complex.h>
#include "quickui.h"

typedef struct {
    int width;
    int height;
    const char *title;
} Win;

Plug plug = {0};
qui_Context ctx = {0};

const char *lib_name = "libplug.so";
void *libplug = NULL;

plug_update_t plug_update = NULL;
plug_init_t plug_init = NULL;

bool reload_libplug(void)
{
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
}
int main(void)
{
    if (!reload_libplug()) return 1;
    Win win = {0};
    win.width = 1400;
    win.height = 900;
    win.title = "Msode";

    int file_counter = 0;
    int item = 0;
    bool requested = false;

    InitWindow(win.width, win.height, win.title);
    InitAudioDevice();

    SetTargetFPS(60);
    String_DA *music_path = init_String_dynamic_array(2);
    qui_init(&ctx, NULL);

    while (!WindowShouldClose()) {
    
        BeginDrawing();
        ClearBackground(CLITERAL(Color) {0x18, 0x18, 0x18, 0xFF});

        if (file_counter == 0) {
            plug_init(&plug, music_path, &file_counter, &ctx);
        }
        if(IsKeyPressed(KEY_R)) if(!reload_libplug()) return 1;

        plug_update(&plug, music_path, &file_counter, &item, &requested, &ctx);

        EndDrawing();
    }
    for (int i = 0; i < file_counter; i++)
    {
        if (plug.music[i].stream.buffer != NULL)  UnloadMusicStream(plug.music[i]);
    }
    
    if (music_path) free_String_DA(music_path);

    CloseAudioDevice();
    CloseWindow();
    
    
    return 0;
}
