#include "jell_effects.hpp"
#include "math.h"

void effect_solid_level(
    Canvas& canvas,
    const AudioFrame& audio)
{

    for (int i = 0; i < 96; i++)
    {
        canvas.ring_pixel_hsv(
            i,
            220.0f,
            1.0f,
            audio.smoothed_level);
    }

    for (int i=0; i < 4; i++)
    {
        for (int j = 0; j < 12; j++)
            canvas.spoke_pixel_hsv(i, j, 220.0f, 1.0f, audio.smoothed_level);
        canvas.noodle_level(i,audio.smoothed_level);
    }

    canvas.show();
}


void effect_output_test(Canvas& canvas)
{
    static int frame = 0;
    static int state = 0;

    if (frame % 120 == 0)
        state++;

    if (state% 3 == 0) {
        for (int i = 0; i < 96; i++)
            canvas.ring_pixel_hsv(i, 0.0f, 1.0f, 1.0f);
        for (int i = 0; i < 4; i++) {
            for (int j=0; j<12; j++) {
                canvas.spoke_pixel_hsv(i, j, 0.0f, 1.0f, 1.0f);
            }
        }
    }
    else if (state% 3 == 1) {
        for (int i = 0; i < 96; i++)
            canvas.ring_pixel_hsv(i, 120.0f, 1.0f, 1.0f);
        for (int i = 0; i < 4; i++) {
            for (int j=0; j<12; j++) {
                canvas.spoke_pixel_hsv(i, j, 120.0f, 1.0f, 1.0f);
            }
        }
    }
    else if (state% 3 == 2) {
        for (int i = 0; i < 96; i++)
            canvas.ring_pixel_hsv(i, 240.0f, 1.0f, 1.0f);
        for (int i = 0; i < 4; i++) {
            for (int j=0; j<12; j++) {
                canvas.spoke_pixel_hsv(i, j, 240.0f, 1.0f, 1.0f);
            }
        }
    }
    

    if (state%4 == 0){
        canvas.noodle_level(0, 1.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state%4 == 1){
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 1.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state%4 == 2){
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 1.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state%4 == 3){
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 1.0f);
    }
    frame ++;
    canvas.show();
}