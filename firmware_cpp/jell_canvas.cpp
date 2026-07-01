#include "jell_canvas.hpp"

Canvas::Canvas(
    LED_String& ring,
    LED_String* spokes,
    PWM_Light* noodles)
    : ring(ring),
      spokes(spokes),
      noodles(noodles)
{
}

void Canvas::show()
{
    ring.paint_string();

    for (int i = 0; i < 4; i++)
        spokes[i].paint_string();
}

void Canvas::clear()
{
    ring.off();
    for (int i = 0; i < 4; i++)
        spokes[i].off();
    show();
    
}

void Canvas::ring_pixel_hsv(int pixel, float h, float s, float v)
{
    ring.write_pixel_hsv(pixel, h, s, v);
}


void Canvas::spoke_pixel_hsv(
    int spoke,
    int pixel,
    float h,
    float s,
    float v)
{
    spokes[spoke].write_pixel_hsv(pixel, h, s, v);
}

void Canvas::noodle_level(int noodle, float level)
{
    noodles[noodle].set_level(level);
}
