#include "dsa.h"
#include "plug.h"
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <assert.h>
#include <string.h>
#include <rlgl.h>
#include "config.h"

// TODO: Not memory safe
static Plug *g_plug = NULL;
static float volume_fade = 0.0f;
static float time_fade = 0.0f;
static float button_fade = 0.0f;

static float volume = 1.0f;

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

void draw_text_raylib(qui_Context* ctx, const char *text, float x, float y) {
    Font *font = (Font *)ctx->font;
    Color font_color = GRAY;
    if (!font){
        DrawText(text, (int)x, (int)y, 20, font_color);
    } 
    else{
        DrawTextEx(*font, text, (Vector2){x, y}, ctx->font_size,ctx->font_spacing, font_color);
    }
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
    
    static unsigned long long call_count = 0;
    static float sample_buffer[N];
    static int buffer_pos = 0;
    static int initialized = 0;
    
    call_count++;
    
    if (!initialized) {
        memset(sample_buffer, 0, sizeof(sample_buffer));
        buffer_pos = 0;
        initialized = 1;
    }

    Frame *fs = (Frame *)buffer;

    for (unsigned int i = 0; i < frames; i++) {
        sample_buffer[buffer_pos] = fs[i].left;
        buffer_pos = (buffer_pos + 1) % N;
        
        if (buffer_pos % (N/4) == 0) {
            float ordered_buffer[N];
            
            for (int j = 0; j < N; j++) {
                int idx = (buffer_pos + j) % N;
                ordered_buffer[j] = sample_buffer[idx];
            }
            
            // Apply Hann window
            for (int k = 0; k < N; k++) {
                float w = 0.5f * (1.0f - cosf(2.0f * PI * k / (N - 1)));
                g_plug->in[k] = ordered_buffer[k] * w;
            }

            fft(g_plug->in, 1, g_plug->out, N);

            g_plug->max_amp = 0.0f;
            for (int k = 1; k < N/2; k++) {
                float a = amp(g_plug->out[k]);
                if (a > g_plug->max_amp) g_plug->max_amp = a;
            }
        }
    }
}

void plug_init(Plug *plug, String_DA *music_path, int *file_counter, Texture2D penger_texture, Font font,qui_Context *ctx)
{
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
    
    if (*file_counter == 0) {
        const char *msg = "Drag Or\n Drop \n Music";
        int tw = MeasureText("Drag Or", 40);
        float font_size = 40;
        float font_space = 2;
        Vector2 position = {(GetScreenWidth() - tw) / 2, GetScreenHeight()/2 - 60 };
        DrawTextEx(font, msg, position, font_size, font_space, WHITE);
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
}

void draw_frequency_bars(Plug *plug, int screen_width, int screen_height, int current_item)
{
    if (current_item < 0 || !IsMusicValid(plug->music[current_item])) return;
    
    float sample_rate = 44100.0f;
    float freq_resolution = sample_rate / N;
    
    float min_freq = 20.0f;
    float max_freq = 20000.0f; 
    
    int num_bars = N/2 - 1;
    float total_width = (float)screen_width;
    float bar_spacing = 2.0f;
    float bar_width = total_width / num_bars;
    
    if (bar_width < 1.0f) {
        bar_width = 1.0f;
        bar_spacing = fmaxf((total_width - (bar_width * num_bars)) / (num_bars - 1), 0.0f);
    }
    
    int bar_count = 0;
    for (int i = 1; i < N/2; i++) { 
        float frequency = i * freq_resolution;
        
        if (frequency < min_freq || frequency > max_freq) continue;
        
        float amplitude = 0.0f;
        if (plug->max_amp > 0.0f) {
            amplitude = amp(plug->out[i]) / plug->max_amp;
        }
        
        // Apply logarithmic scaling
        float log_amplitude = log10f(1.0f + amplitude * 9.0f); 
        
        float bar_height = fmaxf(log_amplitude * screen_height * 0.7f, 2.0f);

        int panel = 100;
        int x = bar_count * (bar_width + bar_spacing);
        int y = screen_height - bar_height - panel;
        
        float hue = fminf((log10f(frequency) - log10f(min_freq)) / (log10f(max_freq) - log10f(min_freq)) * 270.0f, 270.0f);
        Color color = ColorFromHSV(hue, 1.0f, 1.0f);
        
        if (!IsMusicStreamPlaying(plug->music[current_item])) color = GRAY;
        
        DrawRectangle(x, y, (int)bar_width, (int)bar_height, color);
        bar_count++;
    }
}

void plug_update(Plug *plug, String_DA *music_path, int *file_counter, int *item, bool *requested, Font font,qui_Context *ctx)
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
        
        if (!IsMusicStreamPlaying(plug->music[*item])) {
            PlayMusicStream(plug->music[*item]);
        }
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
            Color text_color = {200, 200, 200, (unsigned char)(100 + 155 * (1.0f - volume_fade))};
            float txt_font_size = 20.0f;
            float txt_font_space = 2.0f;
            if (!IsMusicStreamPlaying(plug->music[*item])) color = GRAY;
            if (volume_fade > 0.1f) {
                ctx->cursor_x = volume_x - 55;
                ctx->cursor_y = (int)slider_y + 10;
                qui_slider_float(ctx, "Volume", &volume, 0.0f, 1.0f, 200.0f);
            } else {
                Vector2 vol_pos = {(int)(volume_x + 1), (int)(slider_y + 8) };
                DrawTextEx(font,"Vol", vol_pos , txt_font_size, txt_font_space, text_color);
                
                int vol_bar_width = (int)(30 * volume);
                DrawRectangle((int)(volume_x + 35), (int)(slider_y + (int)txt_font_size - 3), vol_bar_width, 4, color);
                DrawRectangleLines((int)(volume_x + 35), (int)(slider_y + (int)txt_font_size - 3), vol_bar_width, 4, GRAY);
            }

            // TODO: Add slider for time
            Vector2 time_pos = {(int)(time_x + 5), (int)(slider_y + 8) };
            DrawTextEx(font,"Time", time_pos , txt_font_size, txt_font_space, text_color);
            
            float progress = (length > 0) ? (elapsed / length) : 0.0f;
            int progress_width = (int)(200 * progress);
            DrawRectangle((int)(time_x + 50), (int)(slider_y + (int)txt_font_size - 3), progress_width, 4, color);
            DrawRectangleLines((int)(time_x + 50), (int)(slider_y + (int)txt_font_size - 3), 200, 4, GRAY);

            Vector2 pos = { (int)(time_x + 260), (int)(slider_y + (int)txt_font_size - 9) };
            float time_font_size = 18.0f;
            float time_font_space = 2.0f;
            DrawTextEx(font,TextFormat("%.1f/%.1f", elapsed, length) , pos , time_font_size, time_font_space, text_color );
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

    draw_frequency_bars(plug, w, h, *item);

    if (*file_counter > 0) {
        int x = 26;
        int y = h - 90;
        float fontsize = 20.0f;
        
        const char *current_file = get_String_DA(music_path, *item);
        Vector2 pos = { x , y };
        DrawTextEx(font, TextFormat("Playing: %s", current_file) , pos, fontsize, 2.0f, WHITE);
    }
    
    qui_end(ctx);
}
