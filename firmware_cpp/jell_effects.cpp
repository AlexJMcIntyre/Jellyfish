#include "jell_effects.hpp"

#include <array>
#include <cstdio>
#include <numeric>

#include "jell_config.hpp"
#include "jell_fft.hpp"

void effect_miclevelCheck(
    Canvas& canvas,
    const AudioFrame& audio)
{
    printf(">Level: %f, RMS: %f, RMS_Min: %f, RMS_Max: %f, smoothed_peak: %f, smoothed_level: %f\n",
                       audio.level, audio.rms.value, audio.rms.min, audio.rms.max, audio.rms.smoothed_peak, audio.rms.smoothed_level);
    canvas.all_pixels_hsv(220.0f, 1.0f, audio.rms.smoothed_level);
    canvas.all_noodles_level(audio.rms.smoothed_level);
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


        float n = Field::noise(p, 1.0f, audio.rms.smoothed_level + time * .3);

        canvas.ring_pixel_hsv(
            i,
            220.0f + (n * n * 100),
            1.0f,
            audio.rms.smoothed_level);
    }

    for (int s = 0; s < 4; s++)
    {
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            Point3 p = canvas.spoke_position(s, i);

            float n = Field::noise(p, 0.5f, audio.rms.smoothed_level + time * .3);

            canvas.spoke_pixel_hsv(
                s,
                i,
                220.0f + (n * n * 100),
                1.0f,
                audio.rms.smoothed_level * JellConfig::BRIGHTNESS_MODIFIER);
        }
    }

    float pwml = audio.rms.smoothed_level * 2;

    if (pwml > 1.0f)
        pwml = 1.0f;

    canvas.all_noodles_level(pwml);

    canvas.show();
}

void effect_micFft(Canvas& canvas, const AudioFrame& audio, float time)
{
    static int frame = 0;
    static float hue1 = 0.0f;
    static float hue2 = 0.0f;
    JellFFT fft = JellFFT(audio.samples, audio.sample_count);
    std::vector<float> moo = fft.doIt();

    moo.erase(moo.begin());

    constexpr float sample_rate = 36000.0f;

    constexpr size_t fft_size = 256;

    constexpr size_t band_count = 6;

    std::array<float, band_count> bands{};

    for (size_t i = 0; i < moo.size(); ++i)
    {
        const size_t fft_bin = i + 1;

        const float frequency = static_cast<float>(fft_bin) * sample_rate / static_cast<float>(fft_size);

        const float magnitude = moo[i];

        if (frequency < 500.0f)
        {
            bands[0] += magnitude; // Bass: 141–500 Hz
        }
        else if (frequency < 1000.0f)
        {
            bands[1] += magnitude; // Low mids: 500–1000 Hz
        }
        else if (frequency < 2000.0f)
        {
            bands[2] += magnitude; // Mids: 1–2 kHz
        }
        else if (frequency < 4000.0f)
        {
            bands[3] += magnitude; // Upper mids: 2–4 kHz
        }
        else if (frequency < 8000.0f)
        {
            bands[4] += magnitude; // Presence: 4–8 kHz
        }
        else if (frequency <= 18000.0f)
        {
            bands[5] += magnitude; // Treble: 8–18 kHz
        }
    }

    constexpr float max_hue = 240.0f;

    float total_magnitude = 0.0f;

    for (float magnitude : bands)
    {
        total_magnitude += magnitude;
    }

    size_t start_led = 0;
    float cumulative_magnitude = 0.0f;

    if (total_magnitude > 0.0f)
    {
        for (size_t band = 0; band < band_count; ++band)
        {
            cumulative_magnitude += bands[band];

            const size_t end_led =
                (band == band_count - 1)
                    ? JellConfig::NUMBER_LEDS_IN_RING
                    : static_cast<size_t>(
                        cumulative_magnitude /
                        total_magnitude *
                        static_cast<float>(JellConfig::NUMBER_LEDS_IN_RING)
                    );

            const float hue =
                static_cast<float>(band) *
                max_hue /
                static_cast<float>(band_count - 1);

            for (size_t led = start_led; led < end_led; ++led)
            {
                canvas.ring_pixel_hsv(
                led,
                hue,
                1.0f,
                audio.rms.smoothed_level);
            }

            start_led = end_led;
        }
    }

    std::array<size_t, band_count> band_indexes{};

    std::iota(
        band_indexes.begin(),
        band_indexes.end(),
        0
    );

    std::sort(
        band_indexes.begin(),
        band_indexes.end(),
        [&bands](size_t a, size_t b)
        {
            return bands[a] > bands[b];
        }
    );



        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            if (frame == 0 || frame % 120 == 0)
                hue1 = static_cast<float>(band_indexes[0]) * max_hue / static_cast<float>(band_count - 1);
            canvas.spoke_pixel_hsv(0,i,0,1.0f,audio.rms.smoothed_level);
        }
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            if (frame == 0 || frame % 120 == 0)
                hue2 = static_cast<float>(band_indexes[1]) * max_hue / static_cast<float>(band_count - 1);
            canvas.spoke_pixel_hsv(1,i, hue2,1.0f,audio.rms.smoothed_level);
        }
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            canvas.spoke_pixel_hsv(2,i,hue1,1.0f,audio.rms.smoothed_level);
        }
        for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
        {
            canvas.spoke_pixel_hsv(3,i, hue2,1.0f,audio.rms.smoothed_level);
        }


    float pwml = audio.rms.smoothed_level * 2;

    if (pwml > 1.0f)
        pwml = 1.0f;

    canvas.all_noodles_level(pwml);
    frame++;
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
