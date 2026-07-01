#pragma once
#ifndef LED_STRING_HPP
#define LED_STRING_HPP

#include <algorithm>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h" // Ensure this is accessible

struct Particle {
    bool active;
    float x, y, z;      //z is depth here, 0 for a 2d array.
    float vx, vy, vz;   // Velocity vector
    float ax, ay, az;   // Acceleration
    float h, s, v;      // Color
    float innerSize;    // Radius of full intensity
    float outerSize;    // Radius where light hits zero
    float growth;       // Growth/Decay factor for radius
    float h_vel;        // <--- New: Change in Hue per second
    float v_vel;        // <--- New: Change in Brightness per second
};

class LED_String {
public:
 

    LED_String(PIO pio_in, uint sm_in, uint pin_in, int32_t n);
    ~LED_String(); // clean up memory

    
    // coordinate getters
    float get_x(int i) { return posX[i]; }
    float get_y(int i) { return posY[i]; }
    float get_z(int i) { return posZ[i]; }
    
    //void set_pixel_rgb(int index, uint8_t red, uint8_t green, uint8_t blue);
    void blend_pixel_hsv(int index, float h, float s, float v);

    void write_pixel_hsv(int index, float h, float s, float v);
    
    void paint_string();

    // coordinate mapping method
    void map_pixel(int index, float x, float y, float z);
        
    void set_boundary_buffer(float b) { boundary_buffer = b; }
    float get_boundary_buffer() {return boundary_buffer;}
    
    // New method to spawn a sphere
void emit_particle(float x, float y, float z, 
                  float vx, float vy, float vz, 
                  float ax, float ay, float az, 
                  float h, float s, float v, 
                  float h_vel, float v_vel,          // Moved these up
                  float inner, float outer, 
                  float growth = 0.0f);              // Growth at the end

        void simulate_particles(float deltaTime);
        void composite_particles();
        
        
        float decay = 0.8f;
        
        int get_active_particle_count();
        
        // Getters for your effects logic
        float get_min_x() { return minX; }
        float get_max_x() { return maxX; }
        float get_min_y() { return minY; }
        float get_max_y() { return maxY; }
        float get_min_z() { return minZ; }
        float get_max_z() { return maxZ; }
        
        // Quick helpers for "Center" and "Size"
        float get_width()  { return maxX - minX; }
        float get_height() { return maxY - minY; }
        float get_center_x() { return (minX + maxX) * 0.5f; }
        float get_center_y() { return (minY + maxY) * 0.5f; }
        
        // Helper for your spawning logic in main
        //float get_boundary_radius() { return max_led_dist + boundary_buffer; }

        // New Spawners
        // void maintain_ambient_spatials(int target_count, float speed_min = 1.5f, float speed_max = 4.0f);
    
        // void maintain_fire_spatials(int target_count, float vy_min = 3.0f, float vy_max = 6.0f, 
        //                      float hue_start = 55.0f, float h_vel = -90.0f);

        // void maintain_lava_lamp(int target_count, float hueA = 280.0f, float hueB = 30.0f);
        // void maintain_lava_triad(int target_count, float hueA = 280.0f, float hueB = 30.0f, float hueC = 160.0f);
        
        void off();

        void debug_plane(float time);

private:
    PIO pio;
    uint sm;
    uint pin;
    int32_t numLEDs;
    //uint8_t *r, *g, *b;
    float *h_buf, *s_buf, *v_buf; 

    // Coordinate buffers
    float *posX;
    float *posY;
    float *posZ;

    static const int MAX_PARTICLES = 50;
    Particle particles[MAX_PARTICLES];

    void hsv_to_rgb(float h, float s, float v, uint8_t& out_r, uint8_t& out_g, uint8_t& out_b);

    // Canvas Extents
    float minX = 0, maxX = 0;
    float minY = 0, maxY = 0;
    float minZ = 0, maxZ = 0;

    float boundary_buffer = 1.0f; // Extra space for spheres to fade in/out
    
    bool has_custom_mapping = false; //defaults to a unit-spaced 1d line without custom mapping

    void ws2812_init();

    // Internal helper for randomized floats
    float get_rand(float min, float max);

    // New buffers for mixing math
    float *accR, *accG, *accB, *accW;
        
};

class PWM_Light {
public:
    PWM_Light(uint gpio,
              float x,
              float y,
              float z);

    void render_field(float t);
    void set_level(float t);

private:
    uint gpio;
    float posX, posY, posZ;
};

#endif