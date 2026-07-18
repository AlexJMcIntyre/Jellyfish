#include "jell_audio.hpp"
#include "i2s_microphone.pio.h"
#include "math.h"
#include <iostream>
#include <stdio.h>
Microphone::Microphone(int32_t samp_size)
{
    sample_size = samp_size;

    // Set up two buffers for ping-ponging with DMA
    buffer_0 = new int32_t[sample_size];
    buffer_1 = new int32_t[sample_size];

    next_buffer_to_fill = buffer_1;
}

Microphone::~Microphone()
{
    delete[] buffer_0;
    delete[] buffer_1;

    dma_channel_unclaim(dma_chan);
}

void Microphone::init(PIO pio, uint sm, uint pin_bclk, uint pin_din)
{
    audio_input_init(pio, sm, pin_bclk, pin_din);
}

void Microphone::audio_input_init(PIO pio, uint sm, uint pin_bclk, uint pin_din)
{
    audio_pio = pio;
    audio_sm = sm;

    // Load the PIO program
    uint offset = pio_add_program(pio, &i2s_microphone_mono_24_program);

    // Configure the state machine
    i2s_microphone_mono_24_program_init(
        pio,
        audio_sm,
        offset,
        pin_bclk,
        pin_din);

    // Configure DMA
    dma_chan = dma_claim_unused_channel(true);

    dma_channel_config dma_cfg =
        dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(
        &dma_cfg,
        DMA_SIZE_32);

    channel_config_set_read_increment(
        &dma_cfg,
        false);

    channel_config_set_write_increment(
        &dma_cfg,
        true);

    channel_config_set_dreq(
        &dma_cfg,
        pio_get_dreq(pio, audio_sm, false));

    dma_channel_configure(
        dma_chan,
        &dma_cfg,
        buffer_0,
        &audio_pio->rxf[audio_sm],
        sample_size,
        true);
}

AudioFrame Microphone::capture()
{
    // Wait until the current DMA transfer has completed
    dma_channel_wait_for_finish_blocking(dma_chan);

    int32_t* completed_buffer =
        (next_buffer_to_fill == buffer_1)
            ? buffer_0
            : buffer_1;

    // Restart DMA immediately on the other buffer
    dma_channel_set_write_addr(
        dma_chan,
        next_buffer_to_fill,
        true);

    // Swap buffers for next time
    next_buffer_to_fill =
        (next_buffer_to_fill == buffer_0)
            ? buffer_1
            : buffer_0;


    int32_t* samples = completed_buffer;


    // Convert unsigned 24-bit samples to signed
    int64_t sum = 0;
    
    // Pass 1: Convert to signed and calculate mean
    for (int i = 0; i < sample_size; i++)
    {
        // samples[i] -= 0x800000;

        // Shift left to force the 24th bit into the 32nd bit slot,
        // then arithmetic shift right back down to sign-extend automatically.
        samples[i] = (samples[i] << 8) >> 8;

        sum += samples[i];
    }

    // Calculate the DC offset (mean)
    int frame_mean = sum / sample_size;

    for (int i = 0; i < sample_size; i++)
    {
        samples[i] -= frame_mean;
    }

    printf("Max before frame: %f \n", rms_max);
    AudioFrame frame = AudioFrame(samples, sample_size, frame_mean, rms_min, rms_max, smoothed_peak, smoothed_level);
    rms_min = frame.rms_min;
    rms_max = frame.rms_max;
    printf("MAx Accoring to frame: %f \n", frame.rms_max);
    printf("MAx Accoring to mic: %f \n", frame.rms_max);
    smoothed_level = frame.rms_smoothed_level;
    smoothed_peak = frame.rms_smoothed_peak;
    return frame;
}

int Microphone::get_sample_size() const
{
    return sample_size;
}

