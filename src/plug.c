#include "dsa.h"
#include "plug.h"
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <assert.h>
#include <string.h>

// TODO: Not memory safe
static Plug *g_plug = NULL;
static float volume_fade = 0.0f;
static float time_fade = 0.0f;
static float button_fade = 0.0f;



static int qui_mouse_in_area(qui_Context *ctx, float x, float y, float w, float h) {
    int mx = ctx->mouse_pos.x;
    int my = ctx->mouse_pos.y;
    return (mx >= (int)x && mx <= (int)(x+w) && my >= (int)y && my <= (int)(y+h));
}

void draw_rect_raylib(qui_Context *ctx, float x, float y, float width, float height, qui_Color color)
{
    Color col = { color.r, color.g, color.b , color.a };
    DrawRectangle((int)x, (int)y, (int)width, (int)height, col);
}

void draw_text_raylib(qui_Context *ctx, const char *text, float x, float y)
{
    DrawText(text, (int)x, (int)y, 16, WHITE);
}

float text_width_raylib(qui_Context *ctx, const char *text)
{
    return (float)MeasureText(text, 16);
}

float text_height_raylib(qui_Context *ctx, const char *text)
{
    return 16.0f;
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

void plug_init(Plug *plug, String_DA *music_path, int *file_counter, Texture2D penger_texture, qui_Context *ctx)
{
    (void)music_path;
    (void)file_counter;

    g_plug = plug;
    
    plug->max_amp = 0.0f;
    for (int i = 0; i < N; i++) {
        plug->in[i] = 0.0f;
        plug->out[i] = 0.0f + 0.0f*I;
    }

    ctx->draw_rect = draw_rect_raylib;
    ctx->draw_text = draw_text_raylib;
    ctx->text_width = text_width_raylib;
    ctx->text_height = text_height_raylib;
    
    const char *msg = "Drag Or\n Drop \n Music";
    int tw = MeasureText("Drag Or", 40);
    DrawText(msg, (GetScreenWidth() - tw) / 2, GetScreenHeight()/2 - 60, 40, WHITE);
    DrawTexture(penger_texture, GetScreenWidth() / 2, GetScreenHeight() /2, LIGHTGRAY);
    
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

float volume = 1.0f;

void plug_update(Plug *plug, String_DA *music_path, int *file_counter, int *item, bool *requested, qui_Context *ctx)
{
    Vector2 mouse_pos = GetMousePosition();
    qui_mouse_move(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        qui_mouse_down(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        qui_mouse_up(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    }

    qui_begin(ctx, 20.0f, 50.0f);
    
    if (*item >= *file_counter) *item = 0;

    if (!(*requested) && (*file_counter) > 0) {
        AttachAudioStreamProcessor(plug->music[*item].stream, callback);
        PlayMusicStream(plug->music[*item]);
        *requested = true;
    }
    
    if (*file_counter > 0) {
        UpdateMusicStream(plug->music[*item]);
    }
    
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
        if (*file_counter > 0) {
            DetachAudioStreamProcessor(plug->music[*item].stream, callback);
        }
    }
    
    if (*file_counter > 0) {
        float elapsed = GetMusicTimePlayed(plug->music[*item]);            
        float length = GetMusicTimeLength(plug->music[*item]);
        if (elapsed >= length) {
            (*item)++;
            if (*item >= *file_counter) *item = 0;
            *requested = false;
        }

        if(IsMusicValid(plug->music[*item])) {
            float slider_y = (float)GetScreenHeight() - 60.0f;
            float hover_area_height = 35.0f;
            float fade_speed = 8.0f * GetFrameTime();
            
            float volume_x = (float)GetScreenWidth() - 320.0f;
            float volume_w = 250.0f;
            float time_x = 20.0f;
            float time_w = 800.0f;
            float button_x = volume_x - 80.0f;
            float button_w = 60.0f;
            
            int hover_volume = qui_mouse_in_area(ctx, volume_x, slider_y - 20.0f, volume_w, hover_area_height);
            int hover_time = qui_mouse_in_area(ctx, time_x, slider_y - 20.0f, time_w, hover_area_height);
            int hover_button = qui_mouse_in_area(ctx, button_x, slider_y - 20.0f, button_w, hover_area_height);
            
            if (hover_volume || ctx->active_id != 0) {
                volume_fade = fminf(volume_fade + fade_speed, 1.0f);
            } else {
                volume_fade = fmaxf(volume_fade - fade_speed, 0.0f);
            }
            
            if (hover_time || ctx->active_id != 0) {
                time_fade = fminf(time_fade + fade_speed, 1.0f);
            } else {
                time_fade = fmaxf(time_fade - fade_speed, 0.0f);
            }
            
            if (hover_button || ctx->active_id != 0) {
                button_fade = fminf(button_fade + fade_speed, 1.0f);
            } else {
                button_fade = fmaxf(button_fade - fade_speed, 0.0f);
            }

            Color color = BLUE;
            if (!IsMusicStreamPlaying(plug->music[*item])) color = GRAY;
            if (volume_fade > 0.1f) {
                ctx->cursor_x = volume_x;
                ctx->cursor_y = slider_y;
                qui_slider_float(ctx, "Volume", &volume, 0.0f, 1.0f, 200.0f);
            } else {
                Color text_color = {200, 200, 200, (unsigned char)(100 + 155 * (1.0f - volume_fade))};
                DrawText("Vol", (int)(volume_x + 5), (int)(slider_y + 8), 14, text_color);
                
                int vol_bar_width = (int)(30 * volume);
                DrawRectangle((int)(volume_x + 35), (int)(slider_y + 12), vol_bar_width, 4, color);
                DrawRectangleLines((int)(volume_x + 35), (int)(slider_y + 12), 30, 4, GRAY);
            }

            // TODO: Add slider for time
            Color text_color = {200, 200, 200, (unsigned char)(100 + 155 * (1.0f - time_fade))};
            DrawText("Time", (int)(time_x + 5), (int)(slider_y + 8), 14, text_color);
            
            float progress = (length > 0) ? (elapsed / length) : 0.0f;
            int progress_width = (int)(200 * progress);
            DrawRectangle((int)(time_x + 50), (int)(slider_y + 12), progress_width, 4, color);
            DrawRectangleLines((int)(time_x + 50), (int)(slider_y + 12), 200, 4, GRAY);
            
            DrawText(TextFormat("%.1f/%.1f", elapsed, length), (int)(time_x + 260), (int)(slider_y + 8), 12, text_color);
            
        }
    }
    
    if (*file_counter > 0) {
        SetMusicVolume(plug->music[*item], volume);
    }

    if (IsKeyPressed(KEY_SPACE) && *file_counter > 0) {
        if(IsMusicStreamPlaying(plug->music[(*item)])) {
            PauseMusicStream(plug->music[(*item)]);
        } else {
            ResumeMusicStream(plug->music[(*item)]); 
        }
    }
    
    if (IsKeyPressed(KEY_N) && *file_counter > 0) {
        DetachAudioStreamProcessor(plug->music[*item].stream, callback);
        (*item)++;
        if (*item >= *file_counter) *item = 0;
        *requested = false;
    }

    int w = GetRenderWidth();
    int h = GetRenderHeight();
    float cell_width = (float)w / (N/2);

    if (*file_counter > 0 && IsMusicValid(plug->music[*item])) {
        for (size_t i = 0; i < N/2; i++) { 
            float t = 0.0f;
            if (plug->max_amp > 0.0f) {
                t = amp(plug->out[i]) / plug->max_amp;
            }
            
            float bar_height = fmaxf(t * h * 0.7f, 2.0f);
            float bar_width = fmaxf(cell_width - 1.0f, 1.0f);

            int panel = 100;
            int cw = (i * cell_width);
            int ch = h - bar_height - panel;
            Color color = GRAY;
            if (IsMusicStreamPlaying(plug->music[*item])) color = BLUE;
            DrawRectangle(cw , ch, bar_width, bar_height, color);
        }
    }
    
    if (*file_counter > 0) {
        int x = 20;
        int y = 10;
        int font = 16;
        
        const char *current_file = get_String_DA(music_path, *item);
        DrawText(TextFormat("Now Playing: %s", current_file), x, y, font, WHITE);
    }
    
    qui_end(ctx);
}
