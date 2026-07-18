#pragma once
#include "jell_canvas.hpp"
#include "jell_audio.hpp"
#include "jell_field.hpp"

void effect_miclevelCheck(
    Canvas& canvas,
    const AudioFrame& audio);

void effect_LEDchanneltest(Canvas& canvas);

void effect_micNField(Canvas& canvas, const AudioFrame& audio, float time);

void effect_micFft(Canvas& canvas, const AudioFrame& audio, float time);

void effect_ambientNField(Canvas& canvas, float time, float noisescale, float huebase, float huerange, float timescale);

