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


// --- Global State ---
// Initialize LEDs     

LED_String ring(pio0, 1, 2, 96);
LED_String spokes[] = {
    LED_String(pio0, 2, 3, 12),
    LED_String(pio0, 3,  4, 12),
    LED_String(pio1, 0, 5, 12),
    LED_String(pio1, 1, 6, 12)
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

volatile int display_mode = 0;

// --- Main ---

// This runs ONLY on Core 1
void core1_entry() {
    while (true) {
        if (display_mode == 0) {

            AudioFrame audio = mic.capture();

            effect_solid_level(
                canvas,
                audio);


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
    
    while (true)
    {
        tight_loop_contents();
    }
  
}
