#include "jell_effects.hpp"
#include <cstdio>
#include "jell_config.hpp"

void effect_miclevelCheck(
    Canvas& canvas,
    const AudioFrame& audio)
{
    printf(">Level: %f, RMS: %f, RMS_Min: %f, RMS_Max: %f, smoothed_peak: %f, smoothed_level: %f\n",
                       audio.level, audio.rms_rms, audio.rms_min, audio.rms_max, audio.rms_smoothed_peak, audio.rms_smoothed_level);
    canvas.all_pixels_hsv(220.0f, 1.0f, audio.rms_smoothed_level);
    canvas.all_noodles_level(audio.rms_smoothed_level);
    canvas.show();
}

void effect_LEDchanneltest(Canvas& canvas)
{
    static int frame = 0;
    static int state = 0;

    if (frame % 120 == 0)
        state++;

    if (state % 3 == 0)
    {
        canvas.all_pixels_hsv(0.0f, 1.0f, 1.0f);
    }
    else if (state % 3 == 1)
    {
        canvas.all_pixels_hsv(120.0f, 1.0f, 1.0f);
    }
    else if (state % 3 == 2)
    {
        canvas.all_pixels_hsv(240.0f, 1.0f, 1.0f);
    }

    if (state % 4 == 0)
    {
        canvas.noodle_level(0, 1.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state % 4 == 1)
    {
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 1.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state % 4 == 2)
    {
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 1.0f);
        canvas.noodle_level(3, 0.0f);
    }
    else if (state % 4 == 3)
    {
        canvas.noodle_level(0, 0.0f);
        canvas.noodle_level(1, 0.0f);
        canvas.noodle_level(2, 0.0f);
        canvas.noodle_level(3, 1.0f);
    }
    frame++;
    canvas.show();
}

void effect_micNField(Canvas& canvas, const AudioFrame& audio, float time)
{
    // printf(">Level: %f, RMS: %f, RMS_Min: %f, RMS_Max: %f, smoothed_peak: %f, smoothed_level: %f\n",
    //                audio.level, audio.rms_rms, audio.rms_min, audio.rms_max, audio.rms_smoothed_peak, audio.rms_smoothed_level);
    for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_RING; i++)
    {
        Point3 p = canvas.ring_position(i);


        float n = Field::noise(p, 1.0f, audio.rms_smoothed_level + time * .3);

        canvas.ring_pixel_hsv(
            i,
            220.0f + (n * n * 100),
            1.0f,
            audio.rms_smoothed_level);
    }

    for (int s = 0; s < 4; s++)
    {
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            Point3 p = canvas.spoke_position(s, i);

            float n = Field::noise(p, 0.5f, audio.rms_smoothed_level + time * .3);

            canvas.spoke_pixel_hsv(
                s,
                i,
                220.0f + (n * n * 100),
                1.0f,
                audio.rms_smoothed_level * JellConfig::BRIGHTNESS_MODIFIER);
        }
    }

    float pwml = audio.rms_smoothed_level * 2;

    if (pwml > 1.0f)
        pwml = 1.0f;

    canvas.all_noodles_level(pwml);

    canvas.show();
}

void effect_ambientNField(Canvas& canvas, float time, float noisescale, float huebase, float huerange, float timescale)
{
    float field_offset = 1000.0;

    for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_RING; i++)
    {
        Point3 p = canvas.ring_position(i);

        float f_b = Field::noise(p, noisescale, time * timescale);
        float f_h = Field::noise({p.x + field_offset, p.y, p.z}, noisescale, time * timescale);

        canvas.ring_pixel_hsv(
            i,
            huebase + (f_h * f_h - 0.5) * huerange,
            1.0f,
            f_b);
    }

    for (int s = 0; s < 4; s++)
    {
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            Point3 p = canvas.spoke_position(s, i);

            float f_b = Field::noise(p, noisescale, time * timescale);
            float f_h = Field::noise({p.x + field_offset, p.y, p.z}, noisescale, time * timescale);

            canvas.spoke_pixel_hsv(

                s,
                i,
                huebase + (f_h * f_h - 0.5) * huerange,
                1.0f,
                f_b);
        }

        Point3 np = canvas.noodle_position(s);

        float f_b = Field::noise(np, noisescale, time * timescale);


        canvas.noodle_level(s, (f_b * .6f) + 0.4f);
    }

    canvas.show();
}
