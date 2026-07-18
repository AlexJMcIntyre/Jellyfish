#ifndef JELLYOS_AUDIOFRAME_HPP
#define JELLYOS_AUDIOFRAME_HPP
#include <cmath>
#include <cstdint>
#include <cstdio>

struct RmsData
{
    float value;

    float min;

    float max;

    float smoothed_peak;

    float smoothed_level;
};

class AudioFrame
{
public:
    int32_t* samples;
    int sample_count;

    int32_t mean;
    float level;

    RmsData rms{};

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

        float current_rms = sqrtf((float)sum_of_squares / sample_count);


        float decay_rate = 0.002f;

        if (current_rms > max)
        {
            printf("Updating max to rms %f \n", current_rms);
            max = current_rms;
        }
        else
        {
            printf("Updating max based on decay \n");
            printf("current max %f \n", max);
            printf("current rms %f \n", current_rms);
            max += (current_rms - max) * decay_rate;
            printf("new max%f \n", max);
        }

        if (current_rms < min)
            min = current_rms;
        else
            min += (current_rms - min) * decay_rate;

        level = (current_rms - min) / (max - min);
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

        this->rms.min = min;
        this->rms.max = max;
        this->rms.smoothed_peak = smoothed_peak;
        this->rms.smoothed_level = smoothed_level;
        this->rms.value = current_rms;
    }
};
#endif //JELLYOS_AUDIOFRAME_HPP
