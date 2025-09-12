#include "plug.h"
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <complex.h>
#include <signal.h>
#include <unistd.h>
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
static bool window_minimized = false;
static pid_t tray_pid = 0;

void toggle_window(int sig) {
    if (sig == SIGUSR1) {
        window_minimized = !window_minimized;
        if (window_minimized) {
            SetWindowSize(1, 1);
            SetWindowPosition(-1000, -1000);
        } else {
            SetWindowSize(1400, 900);
            SetWindowPosition(100, 100);
        }
    } else if (sig == SIGUSR2) {
        exit(0);
    }
}

void setup_tray_icon() {
    tray_pid = fork();
    if (tray_pid == 0) {
        char menu_cmd[512];
        snprintf(menu_cmd, sizeof(menu_cmd), 
                "Show/Hide!view-restore!kill -USR1 %d|Quit!application-exit!kill -USR2 %d", 
                getppid(), getppid());
        
        execlp("yad", "yad",
               "--notification",
               "--image=resources/penger.png",
               "--text=Msode Music Player",
               "--menu", menu_cmd,
               "--no-middle",
               NULL);
        exit(1); // If execlp fails
    }
}

void cleanup_tray() {
    if (tray_pid > 0) {
        kill(tray_pid, SIGTERM);
    }
}

int main(int argc, char *argv[])
{
    ui.file_counter = 0;
    ui.item = 0;
    ui.requested = false;
    
    signal(SIGUSR1, toggle_window);
    signal(SIGUSR2, toggle_window);
    signal(SIGTERM, cleanup_tray);
    signal(SIGINT, cleanup_tray);
    
    if (!reload_libplug()) return 1;
    Win win = {0};
    win.width = 1400;
    win.height = 900;
    win.title = "Msode";
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    
    InitWindow(win.width, win.height, win.title);
    
    setup_tray_icon();
    
    ui.font = LoadFontEx("resources/InterVariable.ttf", 48, 0, 0); 
    if (ui.font.texture.id == 0) {
        ui.font = LoadFontEx("", 48, 0, 0);
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
    ui.seek_forward_texture = ImageToTexture("resources/seek_forward.png");
    ui.seek_backward_texture = ImageToTexture("resources/seek_backward.png");
    ui.next_texture = ImageToTexture("resources/next.png");
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
        if (!window_minimized) {
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
        } else {
            plug_init(&plug, &ui, &ctx);
            plug_update(&plug, &ui, &ctx);
            usleep(16666); // ~60 FPS equivalent
        }
    }
    
    // Cleanup
    cleanup_tray();
    
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
