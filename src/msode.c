#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_SIZE 20

typedef struct {
    int width;
    int height;
    const char* title;
} Win;

typedef struct {
    int width;
    int height;
    Color text_color;
    Color fg_color;
    Vector2 text_pos;
    float font_size;
    float spacing;
    
} Button;

void draw_button(Font font, Rectangle *rect, Button *button, const char *text)
{
    DrawRectangle(rect->x, rect->y, button->width, button->height, button->fg_color);
    DrawTextEx(font, text, button->text_pos, button->font_size, button->spacing, button->text_color);
}

void init_button(Button *button,int width, int height,
     Color fg_color, Color text_color, Vector2 text_pos, float font_size, float spacing)
{
    button->width = width;
    button->height = height;
    button->text_color = text_color;
    button->fg_color = fg_color;
    button->text_pos = text_pos;
    button->font_size = font_size;
    button->spacing = spacing;
}

int main(void)
{
    Win win = {0};
    win.width = 800;
    win.height = 600;
    win.title = "Msode";

    float volume = 100.0f;
    int second_passed = 0;
    int width = win.width;
    int height = 20;

    Button button;

    Rectangle rec;
    rec.x = 0;
    rec.y = 0;
    rec.width = 40;
    rec.height = 50;


    InitWindow(win.width, win.height, win.title);
    InitAudioDevice();

    // TODO: Tiny file dialog Integration
    Sound sound = LoadSound("test.ogg");
    SetTargetFPS(60);
    // TODO: Make it load system font
    Font font = LoadFont("deps/font/Iosevka-Regular.ttf");

    PlaySound(sound);

    SetMasterVolume(volume);
  
    while(!WindowShouldClose())
    {
        BeginDrawing();
        second_passed += 1;
        if(!IsWindowMaximized()){
            width = GetScreenWidth();
        }
        ClearBackground(WHITE);

        Rectangle rect;
        rect.x = 0;
        rect.y = 0;
        int button_width = 50;
        int button_height = 50;
        Vector2 text_pos;
        text_pos.x = 3;
        text_pos.y = 3;
        
        DrawRectangle(rec.x, rec.y, width, rec.height, GRAY);
        init_button(&button, button_width, button_height, BLUE, BLACK, text_pos, FONT_SIZE, 1.0f);
        if (IsSoundPlaying(sound) == true) draw_button(font, &rect, &button, "pause");
        else draw_button(font, &rect, &button, "play");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (GetMouseX() <= rec.x + width && GetMouseY() <= rec.y + height){
                if(IsSoundPlaying(sound) == true) PauseSound(sound);
                else ResumeSound(sound); 
            }
            
        }
        if (IsKeyPressed(KEY_SPACE)){
            if(IsSoundPlaying(sound) == true) PauseSound(sound);
            else ResumeSound(sound); 
        }
        if (IsKeyPressed(KEY_F1))
        {
            if (GetMasterVolume() == 0.0f) SetMasterVolume(volume);
            else SetMasterVolume(0.0f);
        }
        if (IsKeyPressed(KEY_F2))
        {
            volume -= 5.0f;
            SetMasterVolume(volume);
        }
        if (IsKeyPressed(KEY_F3))
        {
            volume += 5.0f;
            SetMasterVolume(volume);
        }
        
        EndDrawing();
    }


UnloadSound(sound);   
CloseAudioDevice();
CloseWindow();
}
