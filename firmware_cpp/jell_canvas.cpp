#include "jell_canvas.hpp"
#include <algorithm>
#include <cmath>

#include "jell_config.hpp"

Canvas::Canvas(
    LedString& ring,
    LedString* spokes,
    PwmLight* noodles)
    : ring(ring),
      spokes(spokes),
      noodles(noodles)
{
    bounds_min.x = std::min({
        ring.get_min_x(),
        spokes[0].get_min_x(),
        spokes[1].get_min_x(),
        spokes[2].get_min_x(),
        spokes[3].get_min_x(),
        noodles[0].get_x(),
        noodles[1].get_x(),
        noodles[2].get_x(),
        noodles[3].get_x()
    });

    bounds_max.x = std::max({
        ring.get_max_x(),
        spokes[0].get_max_x(),
        spokes[1].get_max_x(),
        spokes[2].get_max_x(),
        spokes[3].get_max_x(),
        noodles[0].get_x(),
        noodles[1].get_x(),
        noodles[2].get_x(),
        noodles[3].get_x()
    });

    bounds_min.y = std::min({
        ring.get_min_y(),
        spokes[0].get_min_y(),
        spokes[1].get_min_y(),
        spokes[2].get_min_y(),
        spokes[3].get_min_y(),
        noodles[0].get_y(),
        noodles[1].get_y(),
        noodles[2].get_y(),
        noodles[3].get_y()
    });

    bounds_max.y = std::max({
        ring.get_max_y(),
        spokes[0].get_max_y(),
        spokes[1].get_max_y(),
        spokes[2].get_max_y(),
        spokes[3].get_max_y(),
        noodles[0].get_y(),
        noodles[1].get_y(),
        noodles[2].get_y(),
        noodles[3].get_y()
    });

    bounds_min.z = std::min({
        ring.get_min_z(),
        spokes[0].get_min_z(),
        spokes[1].get_min_z(),
        spokes[2].get_min_z(),
        spokes[3].get_min_z(),
        noodles[0].get_z(),
        noodles[1].get_z(),
        noodles[2].get_z(),
        noodles[3].get_z()
    });

    bounds_max.z = std::max({
        ring.get_max_z(),
        spokes[0].get_max_z(),
        spokes[1].get_max_z(),
        spokes[2].get_max_z(),
        spokes[3].get_max_z(),
        noodles[0].get_z(),
        noodles[1].get_z(),
        noodles[2].get_z(),
        noodles[3].get_z()
    });
}

Point3 Canvas::ring_position(int pixel) const
{
    return {
        ring.get_x(pixel),
        ring.get_y(pixel),
        ring.get_z(pixel)
    };
}

Point3 Canvas::spoke_position(int spoke, int pixel) const
{
    return {
        spokes[spoke].get_x(pixel),
        spokes[spoke].get_y(pixel),
        spokes[spoke].get_z(pixel)
    };
}

void Canvas::show()
{
    ring.paint_string();

    for (int i = 0; i < 4; i++)
        spokes[i].paint_string();
}

void Canvas::clear()
{
    ring.off();
    for (int i = 0; i < 4; i++)
        spokes[i].off();
    show();
}

void Canvas::ring_pixel_hsv(int pixel, float h, float s, float v)
{
    ring.write_pixel_hsv(pixel, h, s, v);
}

void Canvas::all_pixels_hsv(float h, float s, float v)
{
    for (int i = 0; i < JellConfig::NUMBER_LEDS_IN_RING; i++)
    {
        ring.write_pixel_hsv(i, h, s, v);
    }
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < JellConfig::NUMBER_LEDS_IN_EACH_TENTACLE; j++)
        {
            spokes[i].write_pixel_hsv(j, h, s, v);
        }
    }
}

void Canvas::all_noodles_level(float level)
{
    for (int i = 0; i < 4; i++)
    {
        noodles[i].set_level(level);
    }
}

void Canvas::spoke_pixel_hsv(
    int spoke,
    int pixel,
    float h,
    float s,
    float v)
{
    spokes[spoke].write_pixel_hsv(pixel, h, s, v);
}

void Canvas::noodle_level(int noodle, float level)
{
    noodles[noodle].set_level(level);
}

Point3 Canvas::noodle_position(int noodle)
{
    return {
        noodles[noodle].get_x(),
        noodles[noodle].get_y(),
        noodles[noodle].get_z()
    };
}
