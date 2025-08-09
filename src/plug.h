#ifndef PLUG_H
#define PLUG_H

#define FONT_SIZE 20


#include "raylib.h"
typedef struct {
    int width;
    int height;
    const char* title;
} Win;

typedef struct {
    Texture texture;
    Vector2 pos;
    Color color;
} Button;

void draw_button(Button *button, Texture texture,Vector2 button_pos, Color fg_color);
void update_state(Button *button, Sound sound[], float volume, int item);
#endif //PLUG_H
