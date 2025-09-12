#include "dsa.h"
#include "plug.h"
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <assert.h>
#include <string.h>
#include <raylib.h>
#include "config.h"

static Plug *g_plug = NULL;
static float volume_fade = 0.0f;
static float time_fade = 0.0f;
static float button_fade = 0.0f;
static float state = 0.0f;
static float volume = 0.5f;
static bool fullscreen_mode = false;
static int normal_panel_height = 100;
static int panel_height = 100;
static float skip_rate = 5.0f;
static float hue;

void raylib_draw_image(qui_Context *ctx, qui_Image *image, float x, float y, float w, float h) {
    Texture2D tex = *(Texture2D*)image->data;
    Rectangle src = { 0, 0, (float)image->width, (float)image->height };
    Rectangle dst = { x + w/2.0f, y + h/2.0f, w, h };
    Vector2 origin = { w/2.0f, h/2.0f };
    DrawTexturePro(tex, src, dst, origin, 0.0f, WHITE);
}

static int qui_mouse_in_area(qui_Context *ctx, float x, float y, float w, float h) {
    int mx = ctx->mouse_pos.x;
    int my = ctx->mouse_pos.y;
    return (mx >= (int)x && mx <= (int)(x+w) && my >= (int)y && my <= (int)(y+h));
}

Texture2D ImageToTexture(const char* path) {
    Image img = LoadImage(path);
    Texture2D texture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    return texture;
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
 

static void fft(float in[], int s, float complex out[], int n)
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

static float amp(float complex n)
{
    return sqrtf(crealf(n)*crealf(n) + cimagf(n)*cimagf(n));
}

void callback(void *buffer, unsigned int frames)
{
    if (g_plug == NULL) return;
    
    static float sample_buffer[PLUG_BUF_N];
    static int buffer_pos = 0;
    static int initialized = 0;
    
    if (!initialized) {
        memset(sample_buffer, 0, sizeof(sample_buffer));
        buffer_pos = 0;
        initialized = 1;
    }

    Frame *fs = (Frame *)buffer;
    if (!fs) return;

    for (unsigned int i = 0; i < frames; i++) {
        sample_buffer[buffer_pos] = fs[i].left;
        buffer_pos = (buffer_pos + 1) % PLUG_BUF_N;
        
        if (buffer_pos % (PLUG_BUF_N/8) == 0) {  
            float ordered_buffer[PLUG_BUF_N];
            
            for (int j = 0; j < PLUG_BUF_N; j++) {
                int idx = (buffer_pos + j) % PLUG_BUF_N;
                ordered_buffer[j] = sample_buffer[idx];
            }
            
            for (int k = 0; k < PLUG_BUF_N; k++) {
                float w = 0.5f * (1.0f - cosf(2.0f * PI * k / (PLUG_BUF_N - 1)));
                g_plug->in[k] = ordered_buffer[k] * w;
            }

            fft(g_plug->in, 1, g_plug->out, PLUG_BUF_N);

            g_plug->max_amp = 0.0f;
            for (int k = 1; k < PLUG_BUF_N/2; k++) {
                float a = amp(g_plug->out[k]);
                if (a > g_plug->max_amp) {
                    g_plug->max_amp = a;
                }
            }
        }
    }
}

void plug_init_imp(Plug *plug, Ui *ui, qui_Context *ctx)
{
    g_plug = plug;
    
    static int fft_initialized = 0;
    if (!fft_initialized) {
        plug->max_amp = 0.0f;
        for (int i = 0; i < PLUG_BUF_N; i++) {
            plug->in[i] = 0.0f;
            plug->out[i] = 0.0f + 0.0f*I;
        }
        fft_initialized = 1;
    }

    ctx->draw_rect = draw_rect_raylib;
    ctx->draw_text = draw_text_raylib;
    ctx->text_width = text_width_raylib;
    ctx->text_height = text_height_raylib;
    ctx->draw_image  = raylib_draw_image;
    
    if (ui->file_counter == 0) {
        const char *msg = "Drag Or\n Drop \n Music";
        int tw = MeasureText("Drag Or", 40);
        float font_size = 40;
        float font_space = 2;
        Vector2 position = {(GetScreenWidth() - tw) / 2, GetScreenHeight()/2 - 60 };
        DrawTextEx(ui->font, msg, position, font_size, font_space, WHITE);
        DrawTexture(ui->texture, GetScreenWidth() / 2, GetScreenHeight() /2, LIGHTGRAY);

        if (IsFileDropped()) {
            FilePathList dropped_files = LoadDroppedFiles();
            for(int i = 0; i < (int)dropped_files.count; ++i) {
                if(ui->file_counter < (MAX - 1)) {
                    append_String_DA(ui->music_path, strdup(dropped_files.paths[i]));
                    plug->music[ui->file_counter] = LoadMusicStream(get_String_DA(ui->music_path, ui->file_counter));
                    ui->file_counter++;
                }
            }
            UnloadDroppedFiles(dropped_files);
        } 
    }
}


void draw_frequency_bars_smooth(Plug *plug, int screen_width, int screen_height, int current_item)
{
    if (current_item < 0 || current_item >= MAX || !IsMusicValid(plug->music[current_item])) 
        return;

    static float prev_bar_height[128] = {0};
    float dt = GetFrameTime();

    float sample_rate = 44100.0f;
    float freq_resolution = sample_rate / PLUG_BUF_N;


    int num_display_bars = 64;
    float bar_width = 7.0f;
    float spacing = ((float)screen_width / num_display_bars) - bar_width;

    float min_freq = 20.0f;
    float max_freq = 22000.0f;
    float log_min = log10f(min_freq);
    float log_max = log10f(max_freq);
    float log_range = log_max - log_min;

    float bar_amplitudes[128] = {0};

    for (int bin = 1; bin < PLUG_BUF_N/2; bin++) {
        float freq = bin * freq_resolution;

        freq = fmaxf(freq, min_freq);
        freq = fminf(freq, max_freq);

        float t = (log10f(freq) - log_min) / log_range;
        float fbar = t * (num_display_bars - 1);
        int bar_low = (int)fbar;
        int bar_high = bar_low + 1;
        float weight_high = fbar - bar_low;
        float weight_low = 1.0f - weight_high;

        float a = amp(plug->out[bin]);

        if (bar_low >= 0 && bar_low < num_display_bars)
            bar_amplitudes[bar_low] += a * weight_low;

        if (bar_high >= 0 && bar_high < num_display_bars)
            bar_amplitudes[bar_high] += a * weight_high;
    }

    // Draw bars
    for (int bar = 0; bar < num_display_bars; bar++) {
        float normalized_amplitude = (plug->max_amp > 0.0001f) ? log10f(1.0f + bar_amplitudes[bar]) / log10f(1.0f + plug->max_amp) : 0.0f;
        if (normalized_amplitude > 1.0f) normalized_amplitude = 1.0f;

        float target_height = sqrtf(normalized_amplitude) * (screen_height - panel_height) * 0.8f;
        prev_bar_height[bar] += (target_height - prev_bar_height[bar]) * 12.0f * dt;
        float bar_height = prev_bar_height[bar];
        if (bar_height < 2.0f) bar_height = 2.0f;

        int x = (int)(bar * (bar_width + spacing));
        int y = screen_height - (int)bar_height - panel_height;

        // Color based on frequency
        float center_freq = powf(10.0f, log_min + ((float)bar + 0.5f) / num_display_bars * log_range);

        if (center_freq < 250.0f) hue = 0.0f + (center_freq - min_freq) / (250.0f - min_freq) * 30.0f;
        else if (center_freq < 2000.0f) hue = 30.0f + (center_freq - 250.0f) / (2000.0f - 250.0f) * 90.0f;
        else hue = 120.0f + (center_freq - 2000.0f) / (max_freq - 2000.0f) * 120.0f;

        Color color = ColorFromHSV(hue, 0.8f, 0.9f);
        if (!IsMusicStreamPlaying(plug->music[current_item])) color = GRAY;

        DrawRectangle(x, y, (int)bar_width, (int)bar_height, color);
    }
}


// TODO: FullScreen Adding
void plug_update_imp(Plug *plug, Ui *ui, qui_Context *ctx)
{

    qui_Image play_image  = { &ui->play_texture, ui->play_texture.width,  ui->play_texture.height,  4 };    
    qui_Image pause_image = { &ui->pause_texture, ui->pause_texture.width, ui->pause_texture.height, 4 };
    qui_Image fullscreen_image = { &ui->fullscreen_texture, ui->fullscreen_texture.width, ui->fullscreen_texture.height, 4 };

    qui_Image seek_forward_image  = { &ui->seek_forward_texture, ui->seek_backward_texture.width,  ui->seek_forward_texture.height,  4 };
    qui_Image seek_backward_image  = { &ui->seek_backward_texture, ui->seek_backward_texture.width,  ui->seek_backward_texture.height,  4 };

    qui_Image next_image  = { &ui->next_texture, ui->next_texture.width,  ui->next_texture.height,  4 };
    qui_Image previous_image  = { &ui->previous_texture, ui->previous_texture.width,  ui->previous_texture.height,  4 };

    Vector2 mouse_pos = GetMousePosition();
    qui_mouse_move(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        qui_mouse_down(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        qui_mouse_up(ctx, (int)mouse_pos.x, (int)mouse_pos.y);
    }

    qui_begin(ctx, 20.0f, 50.0f);
    
    if (ui->item >= ui->file_counter) ui->item = 0;

    if (!ui->requested && ui->file_counter > 0) {
        AttachAudioStreamProcessor(plug->music[ui->item].stream, callback);
        
        if (!IsMusicStreamPlaying(plug->music[ui->item])) {
            PlayMusicStream(plug->music[ui->item]);
        }
        ui->requested = true;
    }
    
    if (ui->file_counter > 0) {
        UpdateMusicStream(plug->music[ui->item]);
    }
    
    if (IsFileDropped()) {
        FilePathList dropped_files = LoadDroppedFiles();
        
        for(int i = 0; i < (int)dropped_files.count; ++i) {
            if(ui->file_counter < MAX - 1) {
                append_String_DA(ui->music_path, strdup(dropped_files.paths[i]));
                plug->music[ui->file_counter] = LoadMusicStream(get_String_DA(ui->music_path, ui->file_counter));
                ui->file_counter++;
            }
        }
        UnloadDroppedFiles(dropped_files);
        
        if (ui->item >= ui->file_counter && ui->file_counter > 0) {
            ui->item = ui->file_counter - 1;
        }
    }
    
    if (ui->file_counter > 0) {
        float elapsed = GetMusicTimePlayed(plug->music[ui->item]);            
        float length = GetMusicTimeLength(plug->music[ui->item]);
        
        if (elapsed >= length) {
            DetachAudioStreamProcessor(plug->music[ui->item].stream, callback);
            ui->item++;
            if (ui->item >= ui->file_counter) ui->item = 0;
            ui->requested = false;
        }

        if(!fullscreen_mode) {
            
            if(IsMusicValid(plug->music[ui->item])) {
                float slider_y = (float)GetScreenHeight() - 60.0f;
                float fade_speed = 8.0f * GetFrameTime();

                float max_slider_width = GetScreenWidth() * 0.2f; 
                float slider_width = fminf(250.0f, max_slider_width);
                float volume_x = GetScreenWidth() - slider_width - 20.0f;
                float volume_w = 250.0f;
                float time_x = 20.0f;
                float time_w = 800.0f;
                float button_x = volume_x - 80.0f;
                float button_w = 60.0f;
                
                Color color = ColorFromHSV(hue, 0.8f, 0.9f);
                Color text_color = {200, 200, 200, (unsigned char)(100 + 155 * (1.0f - volume_fade))};
                float txt_font_size = 20.0f;
                float txt_font_space = 2.0f;
                if (!IsMusicStreamPlaying(plug->music[ui->item])) color = GRAY;

                ctx->cursor_x = volume_x - 25;
                ctx->cursor_y = slider_y + 15;
                qui_slider(ctx, "Vol", &volume, 0.0f, 1.0f, 200.0f);

                Vector2 time_pos = {(int)(time_x + 5), (int)(slider_y + 8) };
                DrawTextEx(ui->font,"Time", time_pos , txt_font_size, txt_font_space, text_color);
                
                float progress = (length > 0) ? (elapsed / length) : 0.0f;
                int progress_width = (int)(200 * progress);
                DrawRectangle((int)(time_x + 60), (int)(slider_y + (int)txt_font_size - 3), progress_width, 4, color);
                DrawRectangleLines((int)(time_x + 60), (int)(slider_y + (int)txt_font_size - 3), 200, 4, GRAY);

                Vector2 pos = { (int)(time_x + 270), (int)(slider_y + (int)txt_font_size - 9) };
                float time_font_size = 18.0f;
                float time_font_space = 2.0f;
                int elapsed_min = (int)(elapsed) / 60;
                int elapsed_sec = (int)(elapsed) % 60;
                int length_min = (int)(length) / 60;
                int length_sec = (int)(length) % 60;

                DrawTextEx(ui->font, TextFormat("%02d:%02d / %02d:%02d", elapsed_min, elapsed_sec, length_min, length_sec), pos, time_font_size, time_font_space, text_color);
            }
        }

    }
    
    if (ui->file_counter > 0) {
        SetMusicVolume(plug->music[ui->item], volume);
    }

    if (IsKeyPressed(KEY_SPACE) && ui->file_counter > 0) {
        if(IsMusicStreamPlaying(plug->music[(ui->item)])) {
            PauseMusicStream(plug->music[(ui->item)]);
        } else {
            ResumeMusicStream(plug->music[(ui->item)]); 
        }
    }

    float rate = 0.1f;
    if (IsKeyPressed(KEY_UP)) {
        volume += rate;
        if (volume >= 1.0f) volume = 1.0f;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        volume -= rate;
        if (volume <= 0.0f) volume = 0.0f;
    }

 
    if (IsKeyPressed(KEY_M)){
        if (volume != 0.0f){
            state = volume;
            volume = 0.0f;
        }
        else volume = state;
    }


    if (IsKeyPressed(KEY_RIGHT)){
        SeekMusicStream(plug->music[ui->item],  GetMusicTimePlayed(plug->music[ui->item]) + skip_rate);
    }

    if (IsKeyPressed(KEY_LEFT)){
        SeekMusicStream(plug->music[ui->item],  GetMusicTimePlayed(plug->music[ui->item]) - skip_rate);
    }
    
    if (IsKeyPressed(KEY_N) && ui->file_counter > 0) {
        DetachAudioStreamProcessor(plug->music[ui->item].stream, callback);
        ui->item++;
        if (ui->item >= ui->file_counter) ui->item = 0;
        ui->requested = false;
    }

    int w = GetRenderWidth();
    int h = GetRenderHeight();

    draw_frequency_bars_smooth(plug, w, h, ui->item);

    float button_size = 30.0f;
    float image_size = 25.0f;

    float center_x = GetScreenWidth() / 2.0f;

    ctx->cursor_x = center_x + (button_size / 2.0f) ;
    ctx->cursor_y = GetScreenHeight() - button_size - 20.0f;
    qui_Image *icon = IsMusicStreamPlaying(plug->music[ui->item]) ? &pause_image : &play_image;
    

    if(ui->file_counter > 0 && fullscreen_mode == false) {
        if (qui_image_button(ctx, icon, button_size, button_size, image_size, image_size)) {
            if (IsMusicStreamPlaying(plug->music[ui->item])) {
                PauseMusicStream(plug->music[ui->item]);
            } else {
                ResumeMusicStream(plug->music[ui->item]);
            }
        }


        float post = 35.0f;
        float post_y = 20.0f;
        ctx->cursor_x = center_x + (button_size / 2.0f) + post;
        ctx->cursor_y = GetScreenHeight() - button_size - post_y;
        if (qui_image_button(ctx, &seek_forward_image, button_size, button_size, image_size, image_size)) {
            SeekMusicStream(plug->music[ui->item],  GetMusicTimePlayed(plug->music[ui->item]) + skip_rate);
        }

        ctx->cursor_x = center_x + (button_size / 2.0f) - post;
        ctx->cursor_y = GetScreenHeight() - button_size - post_y;
        if (qui_image_button(ctx, &seek_backward_image, button_size, button_size, image_size, image_size)) {
            SeekMusicStream(plug->music[ui->item],  GetMusicTimePlayed(plug->music[ui->item]) - skip_rate);
        }
    
        post = 70.0f;
        ctx->cursor_x = center_x + (button_size / 2.0f) + post;
        ctx->cursor_y = GetScreenHeight() - button_size - post_y;
        if (qui_image_button(ctx, &next_image, button_size, button_size, image_size, image_size)) {
            DetachAudioStreamProcessor(plug->music[ui->item].stream, callback);
            ui->item++;
            if (ui->item >= ui->file_counter) ui->item = 0;
            ui->requested = false;
        }

        ctx->cursor_x = center_x + (button_size / 2.0f) - post;
        ctx->cursor_y = GetScreenHeight() - button_size - post_y;
        if (qui_image_button(ctx, &previous_image, button_size, button_size, image_size, image_size) && ui->item > 0) {
            DetachAudioStreamProcessor(plug->music[ui->item].stream, callback);
            ui->item--;
            if (ui->item >= ui->file_counter) ui->item = 0;
            ui->requested = false;                

        }
    
    }

    if(ui->file_counter > 0) {
        button_size = 35.0f;
        image_size = 25.0f;
        ctx->cursor_x = GetScreenWidth() - button_size - 10.0f;
        ctx->cursor_y = 10.0f;


        if (qui_image_button(ctx, &fullscreen_image, button_size, button_size, image_size, image_size)) {
            fullscreen_mode = !fullscreen_mode;
            if (fullscreen_mode) {
                panel_height = 0;
            } else {
                panel_height = normal_panel_height;
            }
        }
    }


    if (!fullscreen_mode) {
        static bool is_clicked = false;

        if (ui->file_counter > 0) {
            int x = 26;
            int y = h - 90;
            float fontsize = 20.0f;

            const char *current_file = get_String_DA(ui->music_path, ui->item);
            const char *full_path = current_file;
            const char *display_text = current_file;

            if (qui_mouse_in_area(ctx, x, y, 300, fontsize + 10)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    is_clicked = !is_clicked;
                }
            }

            if (!is_clicked) {
                const char *last_slash = strrchr(current_file, '/');

                if (last_slash != NULL) {
                    display_text = last_slash + 1;
                }
            } else {
                display_text = full_path;
            }

            Vector2 pos = { x, y };
            
            DrawTextEx(ui->font, TextFormat("Playing: %s", display_text), pos, fontsize, 2.0f, WHITE);
        }
    }

    qui_end(ctx);
}
