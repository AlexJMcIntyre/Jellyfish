#pragma once
#include "jell_led.hpp"

class Canvas
{
public:
    Canvas(
        LED_String& ring,
        LED_String* spokes,
        PWM_Light* noodles);

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

private:
    LED_String& ring;
    LED_String* spokes;
    PWM_Light* noodles;
};