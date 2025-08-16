#include "dsa.h"
#include "plug.h"
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <assert.h>
#include <string.h>

// TODO: Not memory safe
static Plug *g_plug = NULL;

void draw_rect_raylib(qui_Context *ctx, float x, float y, float width, float height, qui_Color color)
{
    Color col = { color.r, color.g, color.b , color.a };
    DrawRectangle((int)x, (int)y, (int)width, (int)height, col);
}

void fft(float in[], int s, float complex out[], int n)
{
    assert(n > 0);
    
    if (n == 1) {
        out[0] = in[0];
        return;
    }
    
    fft(in, s*2, out, n/2);
    fft(in + s, s*2, out + n/2, n/2);
    
    for (int f = 0; f < n/2; ++f) {
        float complex tw = cexpf(-I * 2 * PI * f / n) * out[f + n/2];
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
    if (g_plug == NULL) return;
    
    static int sample_index = 0;
    static float sample_buffer[2 * N];

    Frame *fs = (Frame *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        if (sample_index >= N) sample_index = 0;
        
        sample_buffer[sample_index++] = fs[i].left;

        if (sample_index >= N) {
            // Apply Hann window and copy to plug's in array
            for (int k = 0; k < N; k++) {
                float w = 0.5f * (1.0f - cosf(2*PI*k/(N-1)));
                g_plug->in[k] = sample_buffer[k] * w;
            }

            fft(g_plug->in, 1, g_plug->out, N);

            g_plug->max_amp = 0.0f;
            for (int k = 0; k < N; k++) {
                float a = amp(g_plug->out[k]);
                if (a > g_plug->max_amp) g_plug->max_amp = a;
            }
            
            int leftover = sample_index - N;
            if (leftover > 0 && leftover < N) {
                memmove(sample_buffer, sample_buffer + N, leftover * sizeof(float));
                sample_index = leftover;
            } else {
                sample_index = 0;
            }
        }
    }
}

void plug_init(Plug *plug, String_DA *music_path, int *file_counter, qui_Context *ctx)
{
    (void)music_path;
    (void)file_counter;
    (void)ctx;
    
    g_plug = plug;
    
    plug->max_amp = 0.0f;
    for (int i = 0; i < N; i++) {
        plug->in[i] = 0.0f;
        plug->out[i] = 0.0f + 0.0f*I;
    }
    
    const char *msg = "Drag Or\n Drop \n Music";
    int tw = MeasureText("Drag Or", 40);
    DrawText(msg, (GetScreenWidth() - tw) / 2, GetScreenHeight()/2 - 60, 40, WHITE);

    if (IsFileDropped()) {
        FilePathList dropped_files = LoadDroppedFiles();
        for(int i = 0; i < (int)dropped_files.count; ++i) {
            if(*file_counter < (MAX - 1)) {
                append_String_DA(music_path, strdup(dropped_files.paths[i]));
                plug->music[*file_counter] = LoadMusicStream(get_String_DA(music_path, *file_counter));
                (*file_counter)++;
            }
        }
        UnloadDroppedFiles(dropped_files);
    }
}

void plug_update(Plug *plug, String_DA *music_path, int *file_counter, int *item, bool *requested, qui_Context *ctx)
{

    (void)ctx;
    if (*item >= *file_counter) *item = 0;

    if (!(*requested) && (*file_counter) > 0) {
        AttachAudioStreamProcessor(plug->music[*item].stream, callback);
        PlayMusicStream(plug->music[*item]);
        *requested = true;
    }
    
    UpdateMusicStream(plug->music[*item]);
    
    if (IsFileDropped()) {
        *requested = false;
        FilePathList dropped_files = LoadDroppedFiles();
        
        for(int i = 0; i < (int)dropped_files.count; ++i) {
            if(*file_counter < MAX - 1) {
                append_String_DA(music_path, strdup(dropped_files.paths[i]));
                plug->music[*file_counter] = LoadMusicStream(get_String_DA(music_path, *file_counter));
                (*file_counter)++;
            }
        }
        UnloadDroppedFiles(dropped_files);
        DetachAudioStreamProcessor(plug->music[*item].stream, callback);
    }
    
    
    float elapsed = GetMusicTimePlayed(plug->music[*item]);            
    float length = GetMusicTimeLength(plug->music[*item]);
    if (elapsed >= length) {
        (*item)++;
        if (*item >= *file_counter) *item = 0;
        *requested = false;
    }
    
    float volume = 1.0f;
    SetMusicVolume(plug->music[*item], volume);

    if (IsKeyPressed(KEY_SPACE)) {
        if(IsMusicStreamPlaying(plug->music[(*item)])) {
            PauseMusicStream(plug->music[(*item)]);
        } else {
            ResumeMusicStream(plug->music[(*item)]); 
        }
    }
    
    if (IsKeyPressed(KEY_N)) {
        DetachAudioStreamProcessor(plug->music[*item].stream, callback);
        (*item)++;
        if (*item >= *file_counter) *item = 0;
        *requested = false;
    }

    // TOOD: Better UI
    int w = GetRenderWidth();
    int h = GetRenderHeight();
    float cell_width = (float)w / (N/2);

    if (IsMusicValid(plug->music[*item])) {
        // TODO: Button
        for (size_t i = 0; i < N/2; i++) { 
            float t = 0.0f;
            if (plug->max_amp > 0.0f) {
                t = amp(plug->out[i]) / plug->max_amp;
            }
            
            float bar_height = fmaxf(t * h * 0.8f, 2.0f);
            float bar_width = fmaxf(cell_width - 1.0f, 1.0f);

            int pannel = 50;
            int cw = (i * cell_width);
            int ch = h - bar_height - pannel;
            Color color = GRAY;
            if (IsMusicStreamPlaying(plug->music[*item])) color = BLUE;
            DrawRectangle(cw , ch, bar_width, bar_height, color);
        }
    }
    if (*file_counter > 0) {
        int x = 0;
        int y = 10;
        int font = 20;
        int width = GetRenderWidth();
        int height = 10 + font;
        for (int i = 0; i < *file_counter; i++){
            const char *current_file = get_String_DA(music_path, i);
            if (IsMusicStreamPlaying(plug->music[i])) DrawRectangle(x, y, width, height, BLUE); 
            else DrawRectangle(x, y, width, height, GRAY);
            DrawText(TextFormat("Song playing: %s", current_file), x, y, font, WHITE);       
            y += y;
        }
    }
}
