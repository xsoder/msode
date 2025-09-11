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

int main(int argc, char *argv[])
{
    ui.file_counter = 0;
    ui.item = 0;
    ui.requested = false;

    if (!reload_libplug()) return 1;
    Win win = {0};
    win.width = 1400;
    win.height = 900;
    win.title = "Msode";

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    
    InitWindow(win.width, win.height, win.title);
    ui.font = LoadFontEx("/home/xsoder/.fonts/Iosevka-Regular.ttf", 360, 0, 0); 
    if (ui.font.texture.id == 0) {
        ui.font = LoadFontEx("", 360, 0, 0);
        if (ui.font.texture.id == 0) {
            ui.font = GetFontDefault();
        }
    }
    
    SetTextureFilter(ui.font.texture, TEXTURE_FILTER_BILINEAR);
    int font_size = 16;
    int font_spacing = 1;
    InitAudioDevice();
    SetTargetFPS(60);
    
    ui.music_path = init_String_dynamic_array(2);
    Image penger = LoadImage("resources/penger.png");
    ImageResize(&penger, 150, 150);
    ui.texture = LoadTextureFromImage(penger);

    ui.play_texture  = ImageToTexture("resources/play.png");
    ui.pause_texture = ImageToTexture("resources/pause.png");
    ui.fullscreen_texture = ImageToTexture("resources/fullscreen.png");

    ui.seek_forward_texture  = ImageToTexture("resources/seek_forward.png");
    ui.seek_backward_texture = ImageToTexture("resources/seek_backward.png");

    ui.next_texture  = ImageToTexture("resources/next.png");
    ui.previous_texture = ImageToTexture("resources/previous.png");
    qui_init(&ctx, NULL);
    qui_set_font(&ctx, &ui.font, font_size, font_spacing);
    
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (ui.file_counter < MAX - 1) {
                append_String_DA(ui.music_path, strdup(argv[i]));
                plug.music[ui.file_counter] = LoadMusicStream(argv[i]);
                ui.file_counter++;
            }
        }
    }
    
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

    UnloadTexture(ui.texture);
    UnloadTexture(ui.play_texture);
    UnloadTexture(ui.pause_texture);
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
