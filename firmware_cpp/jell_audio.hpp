#pragma once
#include "AudioFrame.hpp"
#include "hardware/pio.h"
#include "hardware/dma.h"

class Microphone
{
public:

    Microphone(int32_t sample_count);
    ~Microphone();

    void init(PIO pio,
              uint sm,
              uint pin_bclk,
              uint pin_din);

    AudioFrame capture();

    int get_sample_size() const;

    float rms_min = 64000.0f;
    float rms_max = 90000.0f;
    float smoothed_peak = 0.0f;
    float smoothed_level = 0.0f;


private:

    void audio_input_init(PIO pio,
                          uint sm,
                          uint pin_bclk,
                          uint pin_din);

    int sample_size;

    int32_t *buffer_0;
    int32_t *buffer_1;
    int32_t *next_buffer_to_fill;

    int dma_chan;

    uint audio_sm;
    PIO audio_pio;

};
