#include <raylib.h>
#include "plug.h"
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX 200
#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])
EXPORT_DA_TYPES()

typedef struct {
    float left;
    float right;
} Frame;

Frame g_frame[4800] = {0};
size_t g_frame_count;
void callback(void *buffer, unsigned int frames)
{
    size_t capacity = ARRAY_LEN(g_frame);
    if(frames <= capacity - g_frame_count){
        memcpy(g_frame + g_frame_count, buffer, sizeof(Frame)*frames);
        g_frame_count += frames;
    }
    else if(frames <= capacity) {
         memmove(g_frame, g_frame + frames, sizeof(Frame)*(capacity - frames));
         memcpy(g_frame + (capacity - frames), buffer, sizeof(Frame)*frames);
     } else {
         memmove(g_frame, buffer, sizeof(Frame)*capacity);
         g_frame_count = capacity;
     }
 }
int main(void)
{
    Music music[MAX] = {0};
    Win win = {0};
    win.width = 800;
    win.height = 800;
    win.title = "Msode";

    float volume = 1.0f;

    int posx = 20;
    int posy = 20;

    int file_counter = 0;
    int item = 0;
    bool requested = false;

    InitWindow(win.width, win.height, win.title);
    InitAudioDevice();

    // TODO: Tiny file dialog Integration
    SetTargetFPS(60);

    String_DA *music_path = init_String_dynamic_array(2);
    while(!WindowShouldClose())
    {

        if (!requested && file_counter > 0) {
            AttachAudioStreamProcessor(music->stream, callback);
            PlayMusicStream(music[item]);
            requested = true;
        }
        UpdateMusicStream(music[item]);
        BeginDrawing();
        ClearBackground(GRAY);

        if(file_counter == 0)
        {
            DrawText("Drag Or\n Drop music", GetScreenWidth()/2, GetScreenHeight()/2 + 10, 40, WHITE);
            if (IsFileDropped())
            {
                FilePathList dropped_files = LoadDroppedFiles();

                for(int i = 0; i < (int)dropped_files.count; ++i)
                {
                    if(file_counter < (MAX - 1))
                    {
                        append_String_DA(music_path, dropped_files.paths[i]);
                        music[i] = LoadMusicStream(get_String_DA(music_path, file_counter));
                        file_counter++;
                    }
                }
                UnloadDroppedFiles(dropped_files);
            }
        }

        else{
            DrawText(get_String_DA(music_path, item), posx, posy, 20,BLACK);
            if (IsFileDropped())
            {
                requested = false;
                FilePathList dropped_files = LoadDroppedFiles();

                for(int i = 0; i < (int)dropped_files.count; ++i)
                {
                    if(file_counter < MAX - 1)
                    {
                        append_String_DA(music_path, dropped_files.paths[i]);
                        music[i] = LoadMusicStream(get_String_DA(music_path, file_counter));
                        file_counter++;
                    }
                }
                UnloadDroppedFiles(dropped_files);
            }
            float elapsed = GetMusicTimePlayed(music[item]);            
            float length = GetMusicTimeLength(music[item]);
            if (elapsed >= length) item++;
            ClearBackground(GRAY);
            SetMusicVolume(music[file_counter], volume);
            Vector2 button_size = { 20, 20 };
            Vector2 pos = { 10 , 10 };

            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (GetMouseX() <= button_size.x + pos.x && GetMouseY() <= pos.y + button_size.y)
                {
                    if(IsMusicStreamPlaying(music[item]) == true) PauseMusicStream(music[item]);
                    else ResumeMusicStream(music[item]); 
                }
                
            }
            if (IsKeyPressed(KEY_SPACE))
            {
                if(IsMusicStreamPlaying(music[item]) == true) PauseMusicStream(music[item]);
                else ResumeMusicStream(music[item]); 
            }
            if (IsKeyPressed(KEY_N)){
                if(IsMusicValid(music[item++]) == true) item++;
                else item = 0;
                requested = false;
            }
        }
        int w = GetScreenWidth();
        int h = GetScreenWidth();
        float cell_width = (float)w/g_frame_count;
        for (size_t i = 0; i < g_frame_count; i++)
        {
            float t = g_frame[i].left;
            if (t > 0){
                DrawRectangle(i*cell_width, h/2 - h/2*t, 1, h/2*t, RED);
            } else {
                DrawRectangle(i*cell_width, h/2, 1, h/2*t, RED);
            }
        }
        EndDrawing();
    }
    for (int i = 0; i < file_counter; i++)
    {
        UnloadMusicStream(music[i]);
    }
    CloseAudioDevice();
    CloseWindow();
}
