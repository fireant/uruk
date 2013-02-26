#ifndef UTILS_H
#define UTILS_H

#include <SDL/SDL.h>

class Fl_Widget;

void close_cb(Fl_Widget* , void* param);
void runGUI(float* alpha, float* beta, bool* exit);
void fill_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel);

#endif // UTILS_H
