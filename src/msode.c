#include <raylib.h>
#include "plug.h"
#include "dsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <complex.h>


EXPORT_DA_TYPES()

#define MAX 200
#define N 256

#define ARRAY_LEN(xs) sizeof(xs)/sizeof(xs[0])

typedef struct {
    float left;
    float right;
} Frame;

float in[N];
float complex out[N];
float max_amp;

void fft(float in[], int s, float complex out[], int n)
{
    float pi = atan2f(1,1)*4;
    assert(n > 0);
    if (n == 1) {
        out[0] = in[0];
        return;
    }
    fft(in, s*2, out, n/2);
    fft(in + s, s*2, out + n/2, n/2);
    for (int f = 0; f < n/2; ++f) {
        float complex tw = cexpf(-I * 2 * pi * f / n) * out[f + n/2];
        float complex e  = out[f];
        out[f] = e + tw;
        out[f + n/2] = e - tw;
    }
}

float amp(float complex n)
{
    return sqrtf(crealf(n)*crealf(n) + cimagf(n)*cimagf(n));
}

void callback(void *buffer, unsigned int frames)
{
    static int sample_index = 0;
    static float sample_buffer[N];
    Frame *fs = buffer;

    for (unsigned int i = 0; i < frames; i++) {
        sample_buffer[sample_index++] = fs[i].left;

        if (sample_index >= N) {
            for (int k = 0; k < N; k++) {
                float w = 0.5f * (1 - cosf(2*M_PI*k/(N-1)));
                in[k] = sample_buffer[k] * w;
            }

            fft(in, 1, out, N);

            max_amp = 0.0f;
            for (int k = 0; k < N; k++) {
                float a = amp(out[k]);
                if (a > max_amp) max_amp = a;
            }
            int leftover = sample_index - N;
            memmove(sample_buffer, sample_buffer + N, leftover * sizeof(float));
            sample_index = leftover;
        }
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
        int w = GetRenderWidth();
        int h = GetRenderHeight();
        float cell_width = (float)w/N;
        for (size_t i = 0; i < N/2; i++) { 
            float t = amp(out[i]) / max_amp;
            DrawRectangle(i * cell_width, h - h * t, cell_width, h * t, GREEN);
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
