#include <raylib.h>
#include "plug.h"
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 200
EXPORT_DA_TYPES()

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
        EndDrawing();
    }
    for (int i = 0; i < file_counter; i++)
    {
        UnloadMusicStream(music[i]);
    }
    CloseAudioDevice();
    CloseWindow();
}
