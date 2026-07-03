#pragma once
#include "jell_canvas.hpp"
#include "jell_audio.hpp"
#include "jell_field.hpp"

void effect_solid_level(
    Canvas& canvas,
    const AudioFrame& audio);

void effect_output_test(Canvas& canvas);

void noise_test(Canvas& canvas, const AudioFrame& audio, float time);
    