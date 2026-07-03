#pragma once
#include "jell_led.hpp"

struct Point3
{
    float x;
    float y;
    float z;
};

class Canvas
{
public:
    Canvas(
        LED_String& ring,
        LED_String* spokes,
        PWM_Light* noodles);

        Point3 ring_position(int pixel) const;
        Point3 spoke_position(int spoke, int pixel) const;

        void clear();

        void show();

        void ring_pixel_hsv(
            int pixel,
            float h,
            float s,
            float v);

        void spoke_pixel_hsv(
            int spoke,
            int pixel,
            float h,
            float s,
            float v);
        
        void noodle_level(
            int noodle,
            float level);

        void all_pixels_hsv(
            float h,
            float s,
            float v);

            void all_noodles_level(
            float level);

    private:
        LED_String& ring;
        LED_String* spokes;
        PWM_Light* noodles;

        Point3 bounds_max;
        Point3 bounds_min;
};