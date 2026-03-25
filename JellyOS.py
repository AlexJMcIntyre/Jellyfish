import plasma
import time
import math
import random
from machine import Pin, PWM


# --- NOODLE LOOPS SETUP ---
n1 = PWM(Pin(12))
n2 = PWM(Pin(13))
n3 = PWM(Pin(14))
n4 = PWM(Pin(15))

NOODLE_DUTY = int(65535 * 1.0) 

for n in (n1, n2, n3, n4):
    n.freq(1000)
    n.duty_u16(NOODLE_DUTY)

# --- RING LED SETUP ---
RING_LEDS = 94
ring = plasma.WS2812(RING_LEDS, 0, 0, 2)
ring.start()

# --- TENTACLE LED SETUP ---
T_LEDS = 12
t1 = plasma.WS2812(T_LEDS, 0, 1, 3)
t2 = plasma.WS2812(T_LEDS, 0, 2, 4)
t3 = plasma.WS2812(T_LEDS, 0, 3, 5)
t4 = plasma.WS2812(T_LEDS, 1, 0, 6)

ts = (t1, t2, t3, t4)
for t in ts:
    t.start()

t_heightmap = [0.0, 1.0, 2.0, 3.0, 2.5, 1.5, 0.5, 0.5, 1.5, 2.5, 3.5, 4.5]
t_heightmap = [h / 4.5 for h in t_heightmap]

BASE_R = 0
BASE_G = 8
BASE_B = 25


def clamp(x):
    return max(0, min(255, int(x)))



# --- RING EFFECT ---

def update_ring(t):
    for i in range(RING_LEDS):
        angle = (i / RING_LEDS) * math.tau
        wave = (math.sin(angle + t * 1.5) + 1) / 2

        r = BASE_R
        g = BASE_G + wave * 120*.6
        b = BASE_B + wave * 60*.6

        ring.set_rgb(i, clamp(r), clamp(g), clamp(b))
        
# --- Tenticles ---

def update_tentacles(strips, heightmap, t, pulses=3, speed=1.0):
    for strip in strips:
        for i, h in enumerate(heightmap):
            wave = math.sin((h * pulses * math.tau) - (t * speed))

            brightness = max(0, wave) ** 2

            r = 0
            g = int(80 * brightness)
            b = int(160 * brightness)

            strip.set_rgb(i, r, g, b)


# --- MAIN LOOP ---
while True:
    t = time.ticks_ms() / 1000

    update_ring(t)
    update_tentacles(ts, t_heightmap, t, pulses=2, speed=4.0)
  
    #time.sleep(1 / 60)
