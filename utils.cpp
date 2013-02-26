#include "utils.h"

void runGUI(float* alpha, float* beta, bool* exit){
    char buffer[20]="0.995";
    char buffer2[20]="0.1";
    Fl_Window* w = new Fl_Window(1800, 100, 330, 190, "Uruk - NSPLab");
    Fl_Button ok(110, 130, 100, 35, "Update");
    Fl_Input input(60, 40, 250, 25,"Alpha:");
    Fl_Input input2(60, 70, 250, 25,"Beta:");
    input.value(buffer);
    input2.value(buffer2);
    w->end();
    w->show();
    w->user_data(exit);
    w->callback((Fl_Callback*)close_cb);

    while (!(*exit)) {
      Fl::wait();
      Fl_Widget *o;
      while (o = Fl::readqueue()) {
        if (o == &ok) {
            strcpy(buffer, input.value());
            (*alpha) = ::atof(buffer);
            strcpy(buffer2, input2.value());
            (*beta) = ::atof(buffer2);
        }
      }
    }
}

void fill_circle(SDL_Surface *surface, int cx, int cy, int radius, Uint32 pixel)
{
    // Note that there is more to altering the bitrate of this
    // method than just changing this value.  See how pixels are
    // altered at the following web page for tips:
    //   http://www.libsdl.org/intro.en/usingvideo.html
    static const int BPP = 4;

    double r = (double)radius;

    for (double dy = 1; dy <= r; dy += 1.0)
    {
        // This loop is unrolled a bit, only iterating through half of the
        // height of the circle.  The result is used to draw a scan line and
        // its mirror image below it.

        // The following formula has been simplified from our original.  We
        // are using half of the width of the circle because we are provided
        // with a center and we need left/right coordinates.

        double dx = floor(sqrt((2.0 * r * dy) - (dy * dy)));
        int x = cx - dx;

        // Grab a pointer to the left-most pixel for each half of the circle
        Uint8 *target_pixel_a = (Uint8 *)surface->pixels + ((int)(cy + r - dy)) * surface->pitch + x * BPP;
        Uint8 *target_pixel_b = (Uint8 *)surface->pixels + ((int)(cy - r + dy)) * surface->pitch + x * BPP;

        for (; x <= cx + dx; x++)
        {
            *(Uint32 *)target_pixel_a = pixel;
            *(Uint32 *)target_pixel_b = pixel;
            target_pixel_a += BPP;
            target_pixel_b += BPP;
        }
    }
}
