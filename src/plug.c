#include "plug.h"

void draw_button(Button *button, Texture texture ,Vector2 button_pos, Color fg_color)
{
    button->texture = texture;
    button->color = fg_color;
    button->pos = button_pos;
    DrawTextureV(button->texture, button->pos, button->color);
}

void update_state(Button *button, Sound sound[], float volume, int item)
{
    ClearBackground(WHITE);
    PlaySound(sound[item]);

    SetMasterVolume(volume);
    
    Vector2 button_size = { 20, 20 };
    Vector2 pos = { 10 , 10 };
    Texture texture;
    if (IsSoundPlaying(sound[item]) == true) draw_button(button, texture , pos, WHITE);
    else draw_button(button, texture, pos, WHITE);

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (GetMouseX() <= button_size.x + pos.x && GetMouseY() <= pos.y + button_size.y)
        {
            if(IsSoundPlaying(sound[item]) == true) PauseSound(sound[item]);
            else ResumeSound(sound[item]); 
        }
        
    }
    if (IsKeyPressed(KEY_SPACE))
    {
        if(IsSoundPlaying(sound[item]) == true) PauseSound(sound[item]);
        else ResumeSound(sound[item]); 
    }
}
