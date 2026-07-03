#include "jell_effects.hpp"
#include "math.h"

void effect_solid_level(
    Canvas& canvas,
    const AudioFrame& audio)
{
    canvas.all_pixels_hsv(220.0f, 1.0f, audio.smoothed_level);
    canvas.all_noodles_level(audio.smoothed_level);       
    canvas.show();
}


void effect_output_test(Canvas& canvas)
{
    static int frame = 0;
    static int state = 0;

    if (frame % 120 == 0)
        state++;

    if (state% 3 == 0) {
        canvas.all_pixels_hsv(0.0f, 1.0f, 1.0f);
    }
    else if (state% 3 == 1) {
        canvas.all_pixels_hsv(120.0f, 1.0f, 1.0f);
    }
    else if (state% 3 == 2) {
        canvas.all_pixels_hsv(240.0f, 1.0f, 1.0f);
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

void noise_test(Canvas& canvas, const AudioFrame& audio, float time)
{
    for (int i = 0; i < 96; i++)
    {
        Point3 p = canvas.ring_position(i);

        float n = Field::noise(p, 1.0f, audio.smoothed_level + time*.3);

        canvas.ring_pixel_hsv(
            i,
            220.0f + (n*n*100),
            1.0f,
            audio.smoothed_level);
    }

    for (int s = 0; s < 4; s++)
    {
        for (int i = 0; i < 12; i++)
        {
            Point3 p = canvas.spoke_position(s, i);

            float n = Field::noise(p, 0.5f, audio.smoothed_level + time*.3);

            canvas.spoke_pixel_hsv(
                s,
                i,
                220.0f + (n*n*100),
                1.0f,
                audio.smoothed_level);
        }
    }

    float pwml = audio.smoothed_level * 2;
    
    if (pwml > 1.0f)
        pwml = 1.0f;
    
    canvas.all_noodles_level(pwml);

    canvas.show();
}
