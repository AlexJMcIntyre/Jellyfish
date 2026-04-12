#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

// Project Headers
#include "ws2812.pio.h" 
#include "i2s_microphone.pio.h"


class Audio {
private:
    int32_t* buffer_0;
    int32_t* buffer_1;
    int32_t* next_buffer_to_fill;
    int32_t dma_chan;
    uint audio_sm;
    PIO audio_pio;

    void audio_input_init(PIO pio, uint sm, uint pin_bclk, uint pin_din) {
        audio_pio = pio;
        //_sm = pio_claim_unused_sm(pio, true);
        //don't do that! Updated to specifically claim the state machine we want, since the others are being used for the LEDs.
        audio_sm = sm;
        
        //load the instructions to the PIO
        uint offset = pio_add_program(pio, &i2s_microphone_mono_24_program);

        //configure the state machine with the right pin mappings and settings for our microphone
        i2s_microphone_mono_24_program_init(pio, audio_sm, offset, pin_bclk, pin_din);

        // Set up DMA to automatically move samples from the PIO RX FIFO to our buffers. Sorry whoever reads this, this is absolute copy-paste witchcraft. 
        dma_chan = dma_claim_unused_channel(true);
        dma_channel_config dma_cfg = dma_channel_get_default_config(dma_chan);
        channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_32);
        channel_config_set_read_increment(&dma_cfg, false);
        channel_config_set_write_increment(&dma_cfg, true);
        channel_config_set_dreq(&dma_cfg, pio_get_dreq(pio, audio_sm, false));

        dma_channel_configure(dma_chan, &dma_cfg, buffer_0, &audio_pio->rxf[audio_sm], Sample_Size, true);
    }

public:
    int32_t Sample_Size; //Public since we need to know it for for things like fft
    // audio sample size, must be a power of 2 for the FFT. Smaller = faster, larger = better frequency resolution
    Audio(int32_t samp_size, PIO pio, uint sm, uint pin_bclk, uint pin_din) {
        Sample_Size = samp_size;
        // set up two buffers for ping-ponging with DMA
        buffer_0 = new int32_t[Sample_Size];
        buffer_1 = new int32_t[Sample_Size];
        next_buffer_to_fill = buffer_1;
        audio_input_init(pio, sm, pin_bclk, pin_din);    
    }

    int32_t* get_audio_buffer() {
        // Wait for the current DMA transfer to finish (i.e., the current buffer is full)
        dma_channel_wait_for_finish_blocking(dma_chan);
        int32_t* processing_buffer = (next_buffer_to_fill == buffer_1) ? buffer_0 : buffer_1;
        
        // Restart DMA immediately
        dma_channel_set_write_addr(dma_chan, next_buffer_to_fill, true);
        
        // Swap for next time
        next_buffer_to_fill = (next_buffer_to_fill == buffer_0) ? buffer_1 : buffer_0;
        
        return processing_buffer;
    }
};

//helper for high pass filter
int32_t high_pass_filter(int32_t sample) {
    float prev_sample = 0, filtered_val = 0;
    bool initialized = false;
    const float alpha = 0.990f;
    float current_sample = (float)sample;
    if (!initialized) { prev_sample = current_sample; initialized = true; return 0; }
    printf("you're wrong!");
    filtered_val = alpha * (filtered_val + current_sample - prev_sample);
    prev_sample = current_sample;
    return (int32_t)filtered_val;
}

float calculate_rms(int32_t* buffer, uint16_t length) {
    // simple RMS volume calculation with high pass filter to remove DC offset. Not currently used but nice for a quick volume-meter or something if we want to add that later.
    int64_t sum_sq = 0;
    for (int i = 0; i < length; i++) {
        // Sign extend 24-bit to 32-bit
        int32_t signed_sample = ((int32_t)(buffer[i] << 8)) >> 8;
        int32_t clean = high_pass_filter(signed_sample);
        sum_sq += ((int64_t)clean * clean);
    }
    return sqrtf((float)sum_sq / length);
}

// loop Pins 12, 13, 14, 15
const uint LED_PINS[] = {12, 13, 14, 15};

void init_dumb_leds() {
    for (int i = 0; i < 4; i++) {
        gpio_set_function(LED_PINS[i], GPIO_FUNC_PWM);
        uint slice_num = pwm_gpio_to_slice_num(LED_PINS[i]);
        
        // Set period of 65535 cycles (standard 16-bit PWM)
        pwm_set_wrap(slice_num, 65535);
        pwm_set_enabled(slice_num, true);
    }
}

void update_dumb_leds() {
    static float phase = 0;
    phase += 0.02f; // Control speed of the ramp here

    for (int i = 0; i < 4; i++) {
        // Offset each LED's phase so they "wave" across the 4 pins
        float brightness = (sinf(phase + (i * 0.5f)) + 1.0f) / 2.0f;
        
        // Scale 0.0-1.0 to the 0-65535 PWM range
        // Square the brightness for a "Gamma Corrected" feel (looks more natural)
        uint16_t level = (uint16_t)(brightness * brightness * 65535.0f);
        
        pwm_set_gpio_level(LED_PINS[i], level);
        //pwm_set_gpio_level(LED_PINS[i], 65500);
    }
}

#define PIR_PIN 9
void init_pir_sensor() {
    gpio_init(PIR_PIN);
    gpio_set_dir(PIR_PIN, GPIO_IN);
    // Pull-down ensures the pin stays at 0V unless the PIR pushes it to 3.3V
    gpio_pull_down(PIR_PIN); 
}

void check_pir_sensor() {
    static bool last_state = false;
    bool current_state = gpio_get(PIR_PIN);

    if (current_state != last_state) {
        if (current_state) {
            printf("Motion Detected!\n");
        } else {
            printf("Sensor Reset\n");
        }
        last_state = current_state;
    }
}

// Set up a WS2812 LED strip on a given pin, pio and state machine
static inline void ws2812_init(PIO pio, uint sm, uint pin) {
    // Some PIO wizardry to set up the state machine for driving WS2812 LEDs. Loads the program to the PIO if it's not already there, configures the pin, and sets the clock divider for 800kHz signal.
    uint offset = pio_add_program(pio, &ws2812_program);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    float div = clock_get_hz(clk_sys) / (800000.0f * 10);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Helper to push to a specific strip
static inline void put_pixel_to_sm(PIO pio, uint sm, uint32_t grb) {
    pio_sm_put_blocking(pio, sm, grb << 8u);
}

// GRB converter for pixels (takes three separate bytes for red, green, blue, and packs them into a single 32-bit integer)
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// FFT (Radix-2), takes an array of samples and transforms it in place to frequency bins.
void fft(std::complex<float>* x, int n) {
    if (n <= 1) return;
    std::complex<float> even[n/2];
    std::complex<float> odd[n/2];
    for (int i = 0; i < n / 2; i++) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }
    fft(even, n / 2);
    fft(odd, n / 2);
    for (int k = 0; k < n / 2; k++) {
        std::complex<float> t = std::exp(std::complex<float>(0, -2 * M_PI * k / n)) * odd[k];
        x[k] = even[k] + t;
        x[k + n / 2] = even[k] - t;
    }
}

#define BTN_A 19
#define BTN_B 20

void init_buttons() {
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A); // Pin sits at 3.3V by default

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
}

void check_buttons() {
    static uint32_t last_press_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Simple Debounce: Only check every 50ms
    if (now - last_press_time > 50) {
        // Buttons are Active Low (0 = Pressed)
        if (!gpio_get(BTN_A)) {
            printf("Button A (GPIO 19) Pressed!\n");
            last_press_time = now;
        }
        if (!gpio_get(BTN_B)) {
            printf("Button B (GPIO 20) Pressed!\n");
            last_press_time = now;
        }
    }
}

// jellyfish hardware test

// Global spoke buffer to store the ACTUAL current Red, Green, Blue of every LED
// 4 spokes * 12 LEDs = 48 pixels. 3 colors each.
static float spoke_canvas[4][12][3] = {0}; 
static float spoke_levels[4] = {0};

void fft_pattern_1(std::complex<float>* fft_results) {
    const float ring_gain = 800.0f;       
    const float spoke_gain = 200.0f;       
    const float decay_rate = 0.5f;   // How fast the "bar" falls
    const float ghost_decay = 0.75f; // How fast the "glow" fades (0.0 to 1.0)

    struct { PIO p; uint sm; } spokes[] = {
        {pio0, 2}, {pio0, 3}, {pio1, 0}, {pio1, 1}
    };

    // --- 1. RING (96 LEDs) ---
    // (Keeping it simple for now, but applying the same logic makes it "shimmer")
    for (int i = 0; i < 96; i++) {
        float mag = std::abs(fft_results[i + 2]);
        uint8_t b = (mag * ring_gain > 30) ? 30 : (uint8_t)(mag * ring_gain);
        put_pixel_to_sm(pio0, 1, urgb_u32(0, b/2, b));
    }

    // --- 2. SPOKES ---
    for (int s = 0; s < 4; s++) {
        float raw_mag = std::abs(fft_results[s * 15 + 5]);
        float target_h = 12.0f * log10f(raw_mag * spoke_gain + 1.0f);

        // Bar Physics
        if (target_h > spoke_levels[s]) spoke_levels[s] = target_h;
        else spoke_levels[s] -= decay_rate;
        if (spoke_levels[s] < 0) spoke_levels[s] = 0;

        for (int i = 0; i < 12; i++) {
            // Determine Target Colors for this LED based on height
            float r_target = 0, g_target = 0, b_target = 0;
            
            if (i < spoke_levels[s]) {
                // Calculate "Full" color for this LED index
                if (i < 5) { g_target = 25 - (i*3); b_target = 40; } // Cyan/Blue
                else if (i < 9) { r_target = (i-5)*10; b_target = 30; } // Purple
                else { r_target = 40; b_target = 10; } // Red

                // Partial brightness for the very top pixel
                float remainder = spoke_levels[s] - i;
                if (remainder < 1.0f) {
                    r_target *= remainder;
                    g_target *= remainder;
                    b_target *= remainder;
                }
            }

            // --- THE GHOSTING MATH ---
            // If the current target is brighter than the canvas, jump to it.
            // If the canvas is brighter, fade it out slowly.
            spoke_canvas[s][i][0] = fmaxf(r_target, spoke_canvas[s][i][0] * ghost_decay);
            spoke_canvas[s][i][1] = fmaxf(g_target, spoke_canvas[s][i][1] * ghost_decay);
            spoke_canvas[s][i][2] = fmaxf(b_target, spoke_canvas[s][i][2] * ghost_decay);

            put_pixel_to_sm(spokes[s].p, spokes[s].sm, urgb_u32(
                (uint8_t)spoke_canvas[s][i][0], 
                (uint8_t)spoke_canvas[s][i][1], 
                (uint8_t)spoke_canvas[s][i][2]
            ));
        }
    }
}

// --- Main Loop ---

int main() {
    // We're probably going to want to debug, right?
    stdio_init_all();

    //start the loops off doing their thing.
    init_dumb_leds();
    
    // Initialize Audio on PIO0, state machine 0, BCLK=Pin16, WS = BCLK + 1, DIN=Pin18
    Audio audio(256, pio0, 0, 16, 18);
    //audio_input_init(pio0, 0, 16, 18);

    // Initialize LEDs     
    ws2812_init(pio0, 1, 2);  // Pin 2, the ring

    ws2812_init(pio0, 2, 3);  // Pin 3, spoke 1
    ws2812_init(pio0, 3, 4);  // Pin 4, spoke 2
    ws2812_init(pio1, 0, 5);  // Pin 5, spoke 3
    ws2812_init(pio1, 1, 6);  // Pin 6, spoke 4

    //motion sensor
    init_pir_sensor();
    
    //set up the button gpios
    init_buttons();       
    
    //set up a complex buffer for the FFT. Complex becasue it keeps the math simple, which literally no-one has ever said before.
    std::complex<float> fft_buffer[audio.Sample_Size];
    // Previous sample and filtered value for the high pass filter, which removes DC offset and makes the visualization more responsive. 
    static int32_t prev_s = 0;
    static float filtered_val = 0;

    while (true) {
        //begin the processing loop
        //Wait for audio data from DMA
        int32_t* raw_samples = audio.get_audio_buffer();

        //Process samples
        for (int i = 0; i < audio.Sample_Size; i++) {
            int32_t s = raw_samples[i];
            
            // Sign extend 24-bit to 32-bit signed
            s = (s << 8) >> 8; 

            // High Pass Filter (Removes DC Offset)
            filtered_val = 0.999f * (filtered_val + (float)s - (float)prev_s);
            prev_s = s;

            // Normalize to small floats for FFT stability
            float normalized = filtered_val / 8388608.0f; 

            // Hanning Window
            float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (audio.Sample_Size - 1)));
            fft_buffer[i] = std::complex<float>(normalized * window, 0.0f);
        }

        // Calculate FFT (Transform the buffer from time domain to frequency domain)
        // the buffer becomes an array of bins, where each bin represents a specific frequency range. Remember we only care about the first half of the bins, since the second half is just a mirror image for real inputs.
        fft(fft_buffer, audio.Sample_Size);

        // Render to Jellfish
        fft_pattern_1(fft_buffer);

        //update the loop leds 
        update_dumb_leds();

        check_pir_sensor();

        check_buttons();
    }
}
