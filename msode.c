#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FONT_SIZE 12

typedef struct {
    int width;
    int height;
    const char* title;
} Win;

void draw_button(Rectangle rect, int width, int height, Color color, const char *text)
{
    DrawRectangle(rect.x, rect.y, width, height, color);
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

    Rectangle rec;
    rec.x = 0;
    rec.y = 0;
    rec.width = 40;
    rec.height = height;

    InitWindow(win.width, win.height, win.title);
    InitAudioDevice();

    Sound sound = LoadSound("test.ogg");
    SetTargetFPS(60);

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
        int button_height = 20;
        DrawRectangle(rec.x, rec.y, width, rec.height, GRAY);
        draw_button(rect, button_width, button_height, GREEN);
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
