#include "jell_effects.hpp"

void effect_solid_level(
    Canvas& canvas,
    const AudioFrame& audio)
{
    canvas.clear();

    for (int i = 0; i < 96; i++)
    {
        canvas.ring_pixel_hsv(
            i,
            0.75f,
            1.0f,
            audio.level);
    }

    canvas.show();
}