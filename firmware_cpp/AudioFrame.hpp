#ifndef JELLYOS_AUDIOFRAME_HPP
#define JELLYOS_AUDIOFRAME_HPP
#include <cmath>
#include <cstdint>
#include <cstdio>

class AudioFrame
{
public:
    int32_t* samples;
    int sample_count;

    int32_t mean;
    float level;

    float rms_rms;

    // RmsDa a* rms;

    float rms_min;

    float rms_max;

    float rms_smoothed_peak;

    float rms_smoothed_level;

    AudioFrame(int32_t* samples, int sample_count, int32_t mean, float min, float max, float smoothed_peak, float smoothed_level)
    {
        this->sample_count = sample_count;
        this->mean = mean;
        this->samples = samples;

        int32_t peak = 0;
        int64_t sum_of_squares = 0;

        for (int i = 0; i < sample_count; i++)
        {
            if (abs(samples[i]) > peak)
                peak = abs(samples[i]);
            sum_of_squares += (int64_t)samples[i] * samples[i];
        }

        // RmsData rms_};

        // float rms = &rms_

        // Calculate RMS

        float rms = sqrtf((float)sum_of_squares / sample_count);


        float decay_rate = 0.002f;


        if (rms > max)
        {
            printf("Updating max to rms %f \n", rms);
            max = rms;
        }
        else
        {
            printf("Updating max based on decay \n");
            printf("current max %f \n", max);
            printf("current rms %f \n", rms);
            max += (rms - max) * decay_rate;
            printf("new max%f \n", max);
        }

        if (rms < min)
            min = rms;
        else
            min += (rms - min) * decay_rate;

        level = (rms - min) / (max - min);
        // Clamp the adaptive range
        if (min > 64000.0f)
            min = 64000.0f;

        if (max < 90000.0f)
            max = 90000.0f;

        float level_decay = 0.99f;
        if (level > smoothed_level)
            smoothed_level = level;
        else
            smoothed_level = smoothed_level * level_decay;

        if (smoothed_level < 0.05)
            smoothed_level = 0.05f;

        float peak_decay = 0.99f;
        if ((level > smoothed_peak) and (level > 0.5))
            smoothed_peak = level;
        else
            smoothed_peak = smoothed_peak * peak_decay;


        this->rms_min = min;
        this->rms_max = max;
        this->rms_smoothed_peak = smoothed_peak;
        this->rms_smoothed_level = smoothed_level;
        this->rms_rms = rms;
    }
};
#endif //JELLYOS_AUDIOFRAME_HPP
