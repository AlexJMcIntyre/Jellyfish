# hardware.py
import array
import math
from machine import Pin, I2S
import rp2

@rp2.asm_pio(sideset_init=rp2.PIO.OUT_LOW, out_shiftdir=rp2.PIO.SHIFT_LEFT, autopull=True, pull_thresh=24)
def ws2812():
    T1, T2, T3 = 2, 5, 3
    wrap_target()
    label("bitloop")
    out(x, 1)               .side(0) [T3 - 1]
    jmp(not_x, "do_zero")   .side(1) [T1 - 1]
    jmp("bitloop")         .side(1) [T2 - 1]
    label("do_zero")
    nop()                   .side(0) [T2 - 1]
    wrap()

class NeoPixelPIO:
    def __init__(self, pin_num, num_leds, sm_id):
        self.sm = rp2.StateMachine(sm_id, ws2812, freq=8_000_000, sideset_base=Pin(pin_num))
        self.sm.active(1)
        self.num_leds = num_leds
        self.buf = array.array("I", [0] * num_leds)
    
    def set_pixel(self, i, color):
        self.buf[i] = (color[1] << 16) | (color[0] << 8) | color[2]
        
    def fill(self, color):
        for i in range(self.num_leds): self.set_pixel(i, color)
        
    def show(self):
        self.sm.put(self.buf, 8)

class AudioVisualizer:
    def __init__(self):
        # Setup I2S
        self.audio_in = I2S(0, sck=Pin(16), ws=Pin(17), sd=Pin(18),
                           mode=I2S.RX, bits=32, format=I2S.MONO,
                           rate=22050, ibuf= 4096)
        
        # Setup LEDs
        self.ring = NeoPixelPIO(2, 96, 1)
        self.drops = [NeoPixelPIO(p, 12, i+2) for i, p in enumerate(range(3, 7))]
        
        self.mic_samples = bytearray(1024)
        self.mic_mv = memoryview(self.mic_samples)
        self.rms = None
        self.max_rms = 1000.0
        self.min_rms = 10.0
        self.decay = .999

    def get_vol(self):
        num_read = self.audio_in.readinto(self.mic_mv)
        if num_read == 0: return 0.0
        
#         samples = array.array("i", self.mic_samples[:num_read])
#         sum_squares = sum((s >> 16)**2 for s in samples)
        samples = array.array("i", self.mic_samples[:num_read])

        vals = [(s >> 14) for s in samples]  # adjust depending on mic
        sum_squares = sum(v*v for v in vals)
        
        self.rms = min(math.sqrt(sum_squares / len(samples)), 4000) #clamp at 4000 to avoid junk data at start
        
  # Auto-Gain Logic
        
  
        if self.rms > self.max_rms:
            self.max_rms = self.rms
        else:
            self.max_rms *= self.decay # Slow decay
            
        if self.rms < self.min_rms:
            self.min_rms = self.rms
        else:
            self.min_rms /= self.decay
        
        # Return normalized volume (0.0 - 1.0)
        return (self.rms - self.min_rms) / (self.max_rms - self.min_rms)

    @staticmethod
    def hsv_to_rgb(h, s, v):
        i = int(h * 6.0); f = (h * 6.0) - i
        p, q, t = int(255*v*(1-s)), int(255*v*(1-f*s)), int(255*v*(1-(1-f)*s))
        v = int(255 * v); i %= 6
        return [(v,t,p), (q,v,p), (p,v,t), (p,q,v), (t,p,v), (v,p,q)][i]
