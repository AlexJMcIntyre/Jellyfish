#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "math.h"

#include "jell_led.hpp"
#include "jell_audio.hpp"
#include "jell_canvas.hpp"
#include "jell_effects.hpp"

//modes
enum class DisplayMode {
    micLevelCheck,
    LEDChannelTest,
    Mic_NField,
    Ambient_Rainbow,   
    Ambient_Deepsea,

    Count
};

volatile DisplayMode display_mode = DisplayMode::Ambient_Deepsea;


// --- Global State ---
// Initialize LEDs     

LED_String ring(pio0, 1, 2, 96, ColourOrder::RGB);
LED_String spokes[] = {
    LED_String(pio0, 2, 3, 12, ColourOrder::RGB),
    LED_String(pio0, 3, 4, 12, ColourOrder::RGB),
    LED_String(pio1, 0, 5, 12, ColourOrder::RGB),
    LED_String(pio1, 1, 6, 12, ColourOrder::RGB)
    };

PWM_Light noodles[] = {
    PWM_Light(12,  0.5f,  0.0f, 0.8f),
    PWM_Light(13,  0.0f,  0.5f, 0.8f),
    PWM_Light(14, -0.5f,  0.0f, 0.8f),
    PWM_Light(15,  0.0f, -0.5f, 0.8f)
};

// Initialize Audio on PIO0, state machine 0, BCLK=Pin16, WS = BCLK + 1, DIN=Pin18
Microphone mic(256);

Canvas canvas(ring, spokes, noodles);

//buttons
constexpr uint BUTTON_PREV = 19;
constexpr uint BUTTON_NEXT = 20;

//button helpers
void next_mode()
{
    int mode = static_cast<int>(display_mode);
    mode = (mode + 1) % static_cast<int>(DisplayMode::Count);
    display_mode = static_cast<DisplayMode>(mode);
}

void previous_mode()
{
    int mode = static_cast<int>(display_mode);
    mode = (mode - 1 + static_cast<int>(DisplayMode::Count))
            % static_cast<int>(DisplayMode::Count);
    display_mode = static_cast<DisplayMode>(mode);
}



// --- Main ---

// This runs ONLY on Core 1
void core1_entry()
{
    while (true)
    {
        switch (display_mode)
        {
            case DisplayMode::micLevelCheck:
            {
                AudioFrame audio = mic.capture();
                printf(">Level: %f, RMS: %f, RMS_Min: %f, RMS_Max: %f, smoothed_peak: %f, smoothed_level: %f\n", audio.level, audio.rms, audio.rms_min, audio.rms_max, audio.smoothed_peak, audio.smoothed_level);

                effect_miclevelCheck(canvas, audio);
                break;
            }

            case DisplayMode::LEDChannelTest:
            {
                effect_LEDchanneltest(canvas);
                break;
            }

            case DisplayMode::Mic_NField:
            {   
                AudioFrame audio = mic.capture();
                float time = time_us_64() * 1e-6f;
                effect_micNField(canvas, audio, time);
                break;
            }

            case DisplayMode::Ambient_Rainbow:
            {
                float time = time_us_64() * 1e-6f;
                effect_ambientNField(canvas, time, 1.0f, 220.0f, 360.0f, 0.15f);
                break;
            }

            case DisplayMode::Ambient_Deepsea:
            {
                float time = time_us_64() * 1e-6f;
                effect_ambientNField(canvas, time, 2.0f, 220.0f, 100.0f, 0.8f);
                break;
            }

            default:
                break;
        }
    }
}


int main() {
    stdio_init_all();
    sleep_ms(1000); 

    mic.init(
    pio0,
    0,
    16,
    18);

    gpio_init(BUTTON_PREV);
    gpio_set_dir(BUTTON_PREV, GPIO_IN);
    gpio_pull_up(BUTTON_PREV);

    gpio_init(BUTTON_NEXT);
    gpio_set_dir(BUTTON_NEXT, GPIO_IN);
    gpio_pull_up(BUTTON_NEXT);

    
    // Map LEDs to 3D/Spatial coordinates
    // Map LEDs around a unit circle
    for (int i = 0; i < 96; i++) {
        float angle = 2.0f * M_PI * (float)i / (float)96;

        float x = cosf(angle);
        float y = sinf(angle);

        ring.map_pixel(i, x, y, 0.0f);
    }

    const float height_map[] = {-1, -2, -2.75, -1.75, -0.5, -0.5, -1.5, -2.5, -3.5, -4.5, -5.5, -6.5};
    for (int i = 0; i < 12; i++) {
        spokes[0].map_pixel(i, 1.0f, 0.0f , height_map[i]);
        spokes[1].map_pixel(i, 0.0f, 1.0f , height_map[i]);
        spokes[2].map_pixel(i, -1.0f, 0.0f, height_map[i]);
        spokes[3].map_pixel(i, 0.0f, -1.0f, height_map[i]);
    }

    multicore_launch_core1(core1_entry);
    
    bool last_prev = false;
    bool last_next = false;

    while (true)
    {
        bool prev = !gpio_get(BUTTON_PREV);
        bool next = !gpio_get(BUTTON_NEXT);

        if (prev && !last_prev)
            previous_mode();

        if (next && !last_next)
            next_mode();

        last_prev = prev;
        last_next = next;

        sleep_ms(20);
    }
  
}
