#include "jell_led.hpp"
#include <math.h>
#include <cstdlib>
#include "hardware/pwm.h"



//Constructor: Allocates memory for frame buffers and initializes PIO

LED_String::LED_String(PIO pio_in, uint sm_in, uint pin_in, int32_t n, ColourOrder order = ColourOrder::GRB) 
    : pio(pio_in), sm(sm_in), pin(pin_in), numLEDs(n), colour_order(order)
{
    h_buf = new float[numLEDs];
    s_buf = new float[numLEDs];
    v_buf = new float[numLEDs];

    // Initialize buffers to zero (black)
    for(int i = 0; i < numLEDs; i++) {
        h_buf[i] = 0.0f; s_buf[i] = 0.0f; v_buf[i] = 0.0f;
    }

    posX = new float[numLEDs];
    posY = new float[numLEDs];
    posZ = new float[numLEDs];

    for(int i = 0; i < numLEDs; i++) {
        posX[i] = (float)i; // Default x is the index
        posY[i] = 0.0f;     // Default y is 0
        posZ[i] = 0.0f;     // Default z is 0
    }

    //Set the initial boundary based on the length of the strip
    //If n=100, the furthest LED is at x=99, so max_dist is 99.
    minX = 0; maxX = (float)(numLEDs - 1);
    minY = maxY = minZ = maxZ = 0;

    ws2812_init();
}

//Destructor: Clean up memory to prevent leaks

LED_String::~LED_String() {
    delete[] h_buf; delete[] s_buf; delete[] v_buf;
    delete[] posX; delete[] posY; delete[] posZ;
}

//Private: Configures the PIO state machine using the generated header
void LED_String::ws2812_init() {
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

void LED_String::write_pixel_hsv(int index, float h, float s, float v) {
    if (index >= 0 && index < numLEDs) {
        h_buf[index] = fmodf(h, 360.0f); // Ensure hue is in bounds
        if (h_buf[index] < 0) h_buf[index] += 360.0f;
        
        s_buf[index] = std::clamp(s, 0.0f, 1.0f);
        v_buf[index] = std::clamp(v, 0.0f, 1.0f);
    }
}


void LED_String::paint_string() {
    for (int i = 0; i < numLEDs; i++) {
        uint8_t r_out, g_out, b_out;
        
        // Convert the "Live" HSV state to RGB just for the hardware
        hsv_to_rgb(h_buf[i], s_buf[i], v_buf[i], r_out, g_out, b_out);

        uint32_t pixel;

        switch (colour_order)
        {
            case ColourOrder::RGB:
                pixel = ((uint32_t)r_out << 16) |
                        ((uint32_t)g_out << 8)  |
                        (uint32_t)b_out;
                break;

            case ColourOrder::GRB:
            default:
                pixel = ((uint32_t)g_out << 16) |
                        ((uint32_t)r_out << 8)  |
                        (uint32_t)b_out;
                break;
            
            case ColourOrder::GBR:
                pixel = ((uint32_t)g_out << 16) |
                        ((uint32_t)b_out << 8)  |
                        (uint32_t)r_out;
                break;
        }

        pio_sm_put_blocking(pio, sm, pixel << 8u);

        // Apply decay to the Value (brightness) for the trail effect
        v_buf[i] *= decay;
        
        // Clean up: if it's basically dark, kill it to prevent "ghosting"
        if (v_buf[i] < 0.001f) v_buf[i] = 0.0f;
    }
    sleep_us(100);
}

// give a pixel a location in 3d space
void LED_String::map_pixel(int index, float x, float y, float z) {
    if (index >= 0 && index < numLEDs) {

        // If this is the first custom map call, reset the default boundary
        if (!has_custom_mapping) {
            minX = maxX = x;
            minY = maxY = y;
            minZ = maxZ = z;
            has_custom_mapping = true;
        }

        posX[index] = x;
        posY[index] = y;
        posZ[index] = z;

        // Calculate distance of this specific LED from origin
        float d = sqrtf(x*x + y*y + z*z);
        
        // Update Extents
        if (x < minX) minX = x; if (x > maxX) maxX = x;
        if (y < minY) minY = y; if (y > maxY) maxY = y;
        if (z < minZ) minZ = z; if (z > maxZ) maxZ = z;
    }
}

/**
 * Utility: Standard HSV to RGB conversion
 */
void LED_String::hsv_to_rgb(float h, float s, float v, uint8_t& out_r, uint8_t& out_g, uint8_t& out_b) {
    float r_f, g_f, b_f;

    if (s == 0) {
        r_f = g_f = b_f = v;
    } else {
        h = fmodf(h, 360.0f) / 60.0f;
        int i = (int)h;
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
            case 0: r_f = v; g_f = t; b_f = p; break;
            case 1: r_f = q; g_f = v; b_f = p; break;
            case 2: r_f = p; g_f = v; b_f = t; break;
            case 3: r_f = p; g_f = q; b_f = v; break;
            case 4: r_f = t; g_f = p; b_f = v; break;
            default: r_f = v; g_f = p; b_f = q; break;
        }
    }

    out_r = (uint8_t)(r_f * 255);
    out_g = (uint8_t)(g_f * 255);
    out_b = (uint8_t)(b_f * 255);
}

// float LED_String::get_rand(float min, float max) {
//     return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
// }

void LED_String::off() {
    // Clear buffers
    for (int i = 0; i < numLEDs; i++) {
        h_buf[i] = 0;
        s_buf[i] = 0;
        v_buf[i] = 0;
    }

    // Push a fully black frame to the LEDs
    for (int i = 0; i < numLEDs; i++) {
        pio_sm_put_blocking(pio, sm, 0);
    }

}


PWM_Light::PWM_Light(uint gpio_in, float x, float y, float z)
    : gpio(gpio_in), posX(x), posY(y), posZ(z)
{
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice, 255);
    pwm_set_enabled(slice, true);
}


void PWM_Light::set_level(float level)
{
    pwm_set_gpio_level(gpio,
    (uint16_t)(level * 255.0f));
}

float LED_String::get_min_x() const
{
    return minX;
}

float LED_String::get_max_x() const
{
    return maxX;
}

float LED_String::get_min_y() const
{
    return minY;
}

float LED_String::get_max_y() const
{
    return maxY;
}

float LED_String::get_min_z() const
{
    return minZ;
}

float LED_String::get_max_z() const
{
    return maxZ;
}

float PWM_Light::get_x() const
{
    return posX;
}

float PWM_Light::get_y() const
{
    return posY;
}

float PWM_Light::get_z() const
{
    return posZ;
}
