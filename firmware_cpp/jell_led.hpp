#pragma once
#ifndef LED_STRING_HPP
#define LED_STRING_HPP

#include <algorithm>
#include <vector>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h" // Ensure this is accessible

// struct Particle {
//     bool active;
//     float x, y, z;      //z is depth here, 0 for a 2d array.
//     float vx, vy, vz;   // Velocity vector
//     float ax, ay, az;   // Acceleration
//     float h, s, v;      // Color
//     float innerSize;    // Radius of full intensity
//     float outerSize;    // Radius where light hits zero
//     float growth;       // Growth/Decay factor for radius
//     float h_vel;        // <--- New: Change in Hue per second
//     float v_vel;        // <--- New: Change in Brightness per second
// };

class LED_String {
public:
 

    LED_String(PIO pio_in, uint sm_in, uint pin_in, int32_t n);
    ~LED_String(); // clean up memory

    
    // coordinate getters
    float get_x(int i) { return posX[i]; }
    float get_y(int i) { return posY[i]; }
    float get_z(int i) { return posZ[i]; }


    void write_pixel_hsv(int index, float h, float s, float v);
    
    void paint_string();

    // coordinate mapping method
    void map_pixel(int index, float x, float y, float z);
    
    float decay = 0.8f;
    
    void off();

    float get_min_x() const;
    float get_max_x() const;

    float get_min_y() const;
    float get_max_y() const;

    float get_min_z() const;
    float get_max_z() const;


private:
    PIO pio;
    uint sm;
    uint pin;
    int32_t numLEDs;
    float *h_buf, *s_buf, *v_buf; 

    // Coordinate buffers
    float *posX;
    float *posY;
    float *posZ;

    void hsv_to_rgb(float h, float s, float v, uint8_t& out_r, uint8_t& out_g, uint8_t& out_b);

    // Canvas Extents
    float minX = 0, maxX = 0;
    float minY = 0, maxY = 0;
    float minZ = 0, maxZ = 0;


    bool has_custom_mapping = false; //defaults to a unit-spaced 1d line without custom mapping

    void ws2812_init();
       
};

class PWM_Light {
public:
    PWM_Light(uint gpio,
              float x,
              float y,
              float z);

    void set_level(float t);

    float get_x() const;
    float get_y() const;
    float get_z() const;

private:
    uint gpio;
    float posX, posY, posZ;
};

#endif