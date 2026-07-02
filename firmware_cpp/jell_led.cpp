#include "jell_led.hpp"
#include <math.h>
#include <cstdlib>
#include "hardware/pwm.h"


//Constructor: Allocates memory for frame buffers and initializes PIO

LED_String::LED_String(PIO pio_in, uint sm_in, uint pin_in, int32_t n) 
    : pio(pio_in), sm(sm_in), pin(pin_in), numLEDs(n) 
{
    // r = new uint8_t[numLEDs];
    // g = new uint8_t[numLEDs];
    // b = new uint8_t[numLEDs];
    h_buf = new float[numLEDs];
    s_buf = new float[numLEDs];
    v_buf = new float[numLEDs];

    // Initialize buffers to zero (black)
    for(int i = 0; i < numLEDs; i++) {
        //r[i] = g[i] = b[i] = 0;
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

    // Set the initial boundary based on the length of the strip
    // If n=100, the furthest LED is at x=99, so max_dist is 99.
    minX = 0; maxX = (float)(numLEDs - 1);
    minY = maxY = minZ = maxZ = 0;

    ws2812_init();
}

//Destructor: Clean up memory to prevent leaks

LED_String::~LED_String() {
    //delete[] r; delete[] g; delete[] b;
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


/**
 * Sets a pixel using HSV values.
 */
void LED_String::blend_pixel_hsv(int index, float h2, float s2, float v2) {
    if (index < 0 || index >= numLEDs || v2 <= 0.0f) return;

    float h1 = h_buf[index];
    float s1 = s_buf[index];
    float v1 = v_buf[index];

    if (v1 <= 0.01f) {
        // If the pixel was dark, just take the new color
        h_buf[index] = h2;
        s_buf[index] = s2;
        v_buf[index] = v2;
        return;
    }

    // 1. Calculate how much the new color "influences" the old one
    // This weights the mix so a bright light overpowers a dim one
    float totalV = v1 + v2;
    float weight = v2 / totalV; 

    // 2. Average Saturation
    s_buf[index] = s1 + (s2 - s1) * weight;

    // 3. Shortest-path Hue Interpolation
    float deltaH = h2 - h1;
    if (deltaH > 180.0f) deltaH -= 360.0f;
    else if (deltaH < -180.0f) deltaH += 360.0f;

    float newH = h1 + (deltaH * weight);
    
    // Wrap the result back to 0-360 range
    if (newH < 0) newH += 360.0f;
    if (newH >= 360.0f) newH -= 360.0f;
    h_buf[index] = newH;

    // 4. Value: Using the Max as you suggested (keeps it vibrant)
    v_buf[index] = std::max(v1, v2); 
    // Alternatively, for a "glow" look, use: v_buf[index] = std::min(1.0f, v1 + v2);
}

/**
 * Direct overwrite: Use this for hard-setting a color, 
 * background fills, or simple 1-particle effects.
 */
void LED_String::write_pixel_hsv(int index, float h, float s, float v) {
    if (index >= 0 && index < numLEDs) {
        h_buf[index] = fmodf(h, 360.0f); // Ensure hue is in bounds
        if (h_buf[index] < 0) h_buf[index] += 360.0f;
        
        s_buf[index] = std::clamp(s, 0.0f, 1.0f);
        v_buf[index] = std::clamp(v, 0.0f, 1.0f);
    }
}


void LED_String::emit_particle(float x, float y, float z, 
                             float vx, float vy, float vz, 
                             float ax, float ay, float az, 
                             float h, float s, float v, 
                             float h_vel, float v_vel, 
                             float inner, float outer, float growth) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            // MATCH EVERY FIELD 1:1 WITH THE STRUCT DEFINITION
            particles[i] = {
                true,                 // active
                x, y, z,              // position
                vx, vy, vz,           // velocity
                ax, ay, az,           // acceleration (THE MISSING PIECE!)
                h, s, v,              // color
                inner, outer, growth, // sizes
                h_vel, v_vel          // color velocities
            };
            return;
        }
    }
}

void LED_String::simulate_particles(float deltaTime = 1.0f) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;

        // 1. Update Velocity based on Acceleration
        particles[i].vx += particles[i].ax * deltaTime;
        particles[i].vy += particles[i].ay * deltaTime;
        particles[i].vz += particles[i].az * deltaTime;

        // 1b. Update Position based on new Velocity
        particles[i].x += particles[i].vx * deltaTime;
        particles[i].y += particles[i].vy * deltaTime;
        particles[i].z += particles[i].vz * deltaTime;

        // 2. Evolution: Grow/Decay
        particles[i].innerSize += particles[i].growth * deltaTime;
        particles[i].outerSize += particles[i].growth * deltaTime;

        // 3. Visuals: Apply Color Change
        particles[i].h += particles[i].h_vel * deltaTime;
        particles[i].v += particles[i].v_vel * deltaTime;

        // Wrap hue (0-360) and clamp brightness (0-1)
        if (particles[i].h < 0) particles[i].h += 360.0f;
        if (particles[i].h >= 360.0f) particles[i].h -= 360.0f;
        if (particles[i].v < 0) particles[i].v = 0;
        if (particles[i].v > 1.0f) particles[i].v = 1.0f;

        // 4. The "Kill Zone" Logic (AABB vs Sphere)
        // We calculate the boundary plus the buffer
        float x_min_kill = minX - boundary_buffer - particles[i].outerSize;
        float x_max_kill = maxX + boundary_buffer + particles[i].outerSize;
        float y_min_kill = minY - boundary_buffer - particles[i].outerSize;
        float y_max_kill = maxY + boundary_buffer + particles[i].outerSize;
        float z_min_kill = minZ - boundary_buffer - particles[i].outerSize;
        float z_max_kill = maxZ + boundary_buffer + particles[i].outerSize;

        bool out_of_bounds = (particles[i].x < x_min_kill || particles[i].x > x_max_kill ||
                              particles[i].y < y_min_kill || particles[i].y > y_max_kill ||
                              particles[i].z < z_min_kill || particles[i].z > z_max_kill);

        // 5. Cleanup: Kill if out of bounds, too small, or completely invisible
        if (out_of_bounds || particles[i].outerSize <= 0.05f || particles[i].v <= 0.01f) {
            particles[i].active = false;
        }
    }
}

// Update render_spheres to stop converting to RGB mid-loop
void LED_String::composite_particles() {
    for (int s = 0; s < MAX_PARTICLES; s++) {
        if (!particles[s].active) continue;

        for (int i = 0; i < numLEDs; i++) {
            float dx = posX[i] - particles[s].x;
            float dy = posY[i] - particles[s].y;
            float dz = posZ[i] - particles[s].z;
            float d = sqrtf(dx*dx + dy*dy + dz*dz);

            if (d < particles[s].outerSize) {
                float intensity = 1.0f;
                if (d > particles[s].innerSize) {
                    intensity = (particles[s].outerSize - d) / (particles[s].outerSize - particles[s].innerSize);
                }
                // Directly set HSV
                blend_pixel_hsv(i, particles[s].h, particles[s].s, particles[s].v * intensity);
            }
        }
    }
}

int LED_String::get_active_particle_count(){
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) count++;
    }
    return count;
}

/**
 * Pushes the buffer to the hardware and applies the decay (fade out)
 */
void LED_String::paint_string() {
    for (int i = 0; i < numLEDs; i++) {
        uint8_t r_out, g_out, b_out;
        
        // Convert the "Live" HSV state to RGB just for the hardware
        hsv_to_rgb(h_buf[i], s_buf[i], v_buf[i], r_out, g_out, b_out);

        uint32_t grb = ((uint32_t)(r_out) << 16) | 
                       ((uint32_t)(g_out) << 8)  | 
                        (uint32_t)(b_out);
        
        pio_sm_put_blocking(pio, sm, grb << 8u);

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

float LED_String::get_rand(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

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

    for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = false;
    }
}


void LED_String::debug_plane(float time)
{
    for (int i = 0; i < numLEDs; i++) {

        // Travelling wave down Z
        float wave = sinf(posZ[i] * 1.5f + time);

        // Convert from -1..1 to 0..1
        float v = (wave + 1.0f) * 0.5f;

        write_pixel_hsv(i, 180.0f, 1.0f, v);
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

void PWM_Light::render_field(float t)
{
    // Placeholder field
    float brightness = (sinf(posZ * 1.5f + t) + 1.0f) * 0.5f;

    pwm_set_gpio_level(gpio,
        (uint16_t)(brightness * 255.0f));
}

void PWM_Light::set_level(float level)
{
    pwm_set_gpio_level(gpio,
    (uint16_t)(level * 255.0f));
}
