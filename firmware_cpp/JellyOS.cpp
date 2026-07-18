#include <cstdio>
#include <cstdlib>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <cmath>

#include "jell_led.hpp"
#include "jell_audio.hpp"
#include "jell_canvas.hpp"
#include "jell_config.hpp"
#include "jell_effects.hpp"

namespace {
    constexpr uint BUTTON_PREV = 19;
    constexpr uint BUTTON_NEXT = 20;
    constexpr uint LOOP_SLEEP_DURATION_MS = 20;
}

volatile auto display_mode = JellConfig::DEFAULT_DISPLAY_MODE;

// --- Global State ---
// Initialize LEDs     
LedString ring(pio0, 1, 2, JellConfig::NUMBER_LEDS_IN_RING, JellConfig::LED_ORDER_RING);
LedString spokes[] = {
    LedString(pio0, 2, 3, JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE, JellConfig::LED_ORDER_TENTACLE),
    LedString(pio0, 3, 4, JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE, JellConfig::LED_ORDER_TENTACLE),
    LedString(pio1, 0, 5, JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE, JellConfig::LED_ORDER_TENTACLE),
    LedString(pio1, 1, 6, JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE, JellConfig::LED_ORDER_TENTACLE)
};

PwmLight noodles[] = {
    PwmLight(12, 0.5f, 0.0f, 0.8f),
    PwmLight(13, 0.0f, 0.5f, 0.8f),
    PwmLight(14, -0.5f, 0.0f, 0.8f),
    PwmLight(15, 0.0f, -0.5f, 0.8f)
};

// Initialize Audio on PIO0, state machine 0, BCLK=Pin16, WS = BCLK + 1, DIN=Pin18
Microphone mic(256);

Canvas canvas(ring, spokes, noodles);

//button helpers
void next_mode()
{
    int mode = static_cast<int>(display_mode);
    mode = (mode + 1) % static_cast<int>(JellConfig::DisplayMode::Count);
    display_mode = static_cast<JellConfig::DisplayMode>(mode);
}

void previous_mode()
{
    int mode = static_cast<int>(display_mode);
    mode = (mode - 1 + static_cast<int>(JellConfig::DisplayMode::Count)) % static_cast<int>(JellConfig::DisplayMode::Count);
    display_mode = static_cast<JellConfig::DisplayMode>(mode);
}


// --- Main ---

// This runs ONLY on Core 1
[[noreturn]] void core1_entry()
{
    while (true)
    {
        switch (display_mode)
        {
        case JellConfig::DisplayMode::micLevelCheck:
            {
                AudioFrame audio = mic.capture();

                effect_miclevelCheck(canvas, audio);
                break;
            }

        case JellConfig::DisplayMode::LEDChannelTest:
            {
                effect_LEDchanneltest(canvas);
                break;
            }

        case JellConfig::DisplayMode::Mic_NField:
            {
                AudioFrame audio = mic.capture();
                float time = time_us_64() * 1e-6f;
                effect_micNField(canvas, audio, time);
                break;
            }

        case JellConfig::DisplayMode::Ambient_Rainbow:
            {
                float time = time_us_64() * 1e-6f;
                effect_ambientNField(canvas, time, 1.0f, 220.0f, 360.0f, 0.15f);
                break;
            }

        case JellConfig::DisplayMode::Ambient_Deepsea:
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


[[noreturn]] int main()
{
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
    for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_RING; i++)
    {
        float angle = 2.0f * M_PI * (float)i / (float)JellConfig::NUMBER_LEDS_IN_RING;

        float x = cosf(angle);
        float y = sinf(angle);

        ring.map_pixel(i, x, y, 0.0f);
    }

    for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; i++)
    {
        constexpr float height_map[] = {-1, -2, -2.75, -1.75, -0.5, -0.5, -1.5, -2.5, -3.5, -4.5, -5.5, -6.5};
        spokes[0].map_pixel(i, 1.0f, 0.0f, height_map[i]);
        spokes[1].map_pixel(i, 0.0f, 1.0f, height_map[i]);
        spokes[2].map_pixel(i, -1.0f, 0.0f, height_map[i]);
        spokes[3].map_pixel(i, 0.0f, -1.0f, height_map[i]);
    }

    multicore_launch_core1(core1_entry);

    bool last_prev = false;
    bool last_next = false;

    while (true)
    {
        const bool prev = !gpio_get(BUTTON_PREV);
        const bool next = !gpio_get(BUTTON_NEXT);

        if (prev && !last_prev)
        {
            previous_mode();
        }

        if (next && !last_next)
        {
            next_mode();
        }

        last_prev = prev;
        last_next = next;

        sleep_ms(LOOP_SLEEP_DURATION_MS);
    }
}
