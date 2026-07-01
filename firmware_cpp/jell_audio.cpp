#include "jell_audio.hpp"
#include "i2s_microphone.pio.h"
#include "math.h"

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

    //return completed_buffer;
    AudioFrame frame;

    frame.samples = completed_buffer;
    int32_t* samples = frame.samples;

    frame.sample_count = sample_size;

    // Convert unsigned 24-bit samples to signed
    int64_t sum = 0;
    
    // Pass 1: Convert to signed and calculate mean
    for (int i = 0; i < frame.sample_count; i++)
    {
        samples[i] -= 0x800000;
        sum += samples[i];
    }

    // Calculate the DC offset (mean)
    frame.mean = sum / frame.sample_count;

    // Pass 2: Remove DC offset and find peak
    int32_t peak = 0;
    int64_t sum_of_squares = 0;
    for (int i = 0; i < frame.sample_count; i++)
    {
        samples[i] -= frame.mean;
        if (abs(samples[i]) > peak)
            peak = abs(samples[i]);
        sum_of_squares += (int64_t)samples[i] * samples[i];
    }

    // Calculate RMS
    frame.rms = sqrtf((float)sum_of_squares / frame.sample_count);

    if (frame.rms < rms_min)
        rms_min = frame.rms;
    else
        rms_min += (frame.rms - rms_min) * 0.0005f;


    if (frame.rms > rms_max)
        rms_max = frame.rms;
    else
        rms_max -= (rms_max - frame.rms) * 0.0005f;

    //calculate the level
    frame.level =
    (frame.rms - rms_min) /
    (rms_max - rms_min);
    

    if (frame.level < 0.0f)
    frame.level = 0.0f;

    if (frame.level > 1.0f)
    frame.level = 1.0f;

    frame.level = sqrtf(frame.level);

    return frame;

}

int Microphone::get_sample_size() const
{
    return sample_size;
}

