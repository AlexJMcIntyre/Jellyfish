#pragma once
#ifndef JELL_PMW_LIGHT_HPP
#define JELL_PMW_LIGHT_HPP

#include "pico/stdlib.h"
#include "hardware/pwm.h"

class PwmLight
{
public:
    PwmLight(uint gpio,
             float x,
             float y,
             float z) : gpio(gpio), posX(x), posY(y), posZ(z)
    {
        gpio_set_function(gpio, GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(gpio);
        pwm_set_wrap(slice, 255);
        pwm_set_enabled(slice, true);
    }

    void set_level(float level)
    {
        pwm_set_gpio_level(gpio, (uint16_t)(level * 255.0f));
    }

    float get_x() const
    {
        return posX;
    }

    float get_y() const
    {
        return posY;
    }

    float get_z() const
    {
        return posZ;
    }

private:
    uint gpio;
    float posX, posY, posZ;
};

#endif
