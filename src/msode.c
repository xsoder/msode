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
#include "config.h"
#include "hotreload.h"

typedef struct {
    int width;
    int height;
    const char *title;
} Win;

Plug plug = {0};
Ui ui = {0};
qui_Context ctx = {0};

int main(void)
{
    if (!reload_libplug()) return 1;
    Win win = {0};
    win.width = 1400;
    win.height = 900;
    win.title = "Msode";

    ui.file_counter = 0;
    ui.item = 0;
    ui.requested = false;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(win.width, win.height, win.title);
    ui.font = LoadFont(FONT);
    int font_size = 18;
    int font_spacing = 1;
    InitAudioDevice();
    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(60);
    
    ui.music_path = init_String_dynamic_array(2);
    Image penger = LoadImage("resources/penger.png");
    ImageResize(&penger, 150, 150);
    ui.texture = LoadTextureFromImage(penger);

    // TODO: system wide font install

    qui_init(&ctx, NULL);
    qui_set_font(&ctx, &ui.font, font_size, font_spacing);
        
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);

        plug_init(&plug, &ui, &ctx);
        
        if(IsKeyPressed(KEY_R)) {
            bool was_playing = false;
            if (ui.file_counter > 0 && ui.requested) {
                was_playing = IsMusicStreamPlaying(plug.music[ui.item]);
                DetachAudioStreamProcessor(plug.music[ui.item].stream, NULL);
                ui.requested = false;
            }
            
            if(!reload_libplug()) return 1;
            
            plug_init(&plug, &ui, &ctx);
            
            if (ui.file_counter > 0) {
                ui.requested = false;
            }
        }

        plug_update(&plug, &ui, &ctx);

        EndDrawing();
    }
    
    for (int i = 0; i < ui.file_counter; i++) {
        if (plug.music[i].stream.buffer != NULL) {
            DetachAudioStreamProcessor(plug.music[i].stream, NULL);
            UnloadMusicStream(plug.music[i]);
        }
    }
    
    if (ui.music_path) free_String_DA(ui.music_path);

    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
