from machine import Pin, SPI, PWM
import framebuf
import time
import math

# ==========================================
# RASPBERRY PI PICO - MINI OSCILLOSCOPE
# Font sangat kecil + Dark Mode
# ==========================================
W = 160
H = 128

tft = SPI(0, baudrate=24_000_000, polarity=0, phase=0,
          sck=Pin(18), mosi=Pin(19), miso=None)
dc  = Pin(16, Pin.OUT)
cs  = Pin(17, Pin.OUT, value=1)
rst = Pin(20, Pin.OUT)

clk    = Pin(10, Pin.IN, Pin.PULL_UP)
dt     = Pin(11, Pin.IN, Pin.PULL_UP)
enc_sw = Pin(12, Pin.IN, Pin.PULL_UP)
button = Pin(13, Pin.IN, Pin.PULL_UP)

led_r = PWM(Pin(2)); led_r.freq(1000)
led_g = PWM(Pin(3)); led_g.freq(1000)
led_b = PWM(Pin(4)); led_b.freq(1000)

buf = bytearray(W * H * 2)
fb  = framebuf.FrameBuffer(buf, W, H, framebuf.RGB565)

# Dark Mode colors
BLACK  = 0x0000
WHITE  = 0xFFFF
CYAN   = 0x07FF
GREEN  = 0x07E0
YELLOW = 0xFFE0
RED    = 0xF800
GRAY   = 0x7BEF
DARK   = 0x2104
WAVE   = 0x07FF

# ==========================================
# FONT SANGAT KECIL 4x6
# ==========================================
FONT4 = {
    '0': [0x1E,0x29,0x25,0x1E],
    '1': [0x00,0x22,0x3F,0x20],
    '2': [0x32,0x29,0x25,0x22],
    '3': [0x11,0x21,0x25,0x1B],
    '4': [0x0C,0x0A,0x3F,0x08],
    '5': [0x17,0x25,0x25,0x19],
    '6': [0x1E,0x25,0x25,0x18],
    '7': [0x01,0x39,0x05,0x03],
    '8': [0x1A,0x25,0x25,0x1A],
    '9': [0x06,0x25,0x25,0x1E],
    'A': [0x3E,0x09,0x09,0x3E],
    'B': [0x3F,0x25,0x25,0x1A],
    'C': [0x1E,0x21,0x21,0x12],
    'D': [0x3F,0x21,0x21,0x1E],
    'E': [0x3F,0x25,0x25,0x21],
    'F': [0x3F,0x05,0x05,0x01],
    'G': [0x1E,0x21,0x25,0x1D],
    'H': [0x3F,0x04,0x04,0x3F],
    'I': [0x00,0x21,0x3F,0x21],
    'J': [0x10,0x20,0x21,0x1F],
    'K': [0x3F,0x04,0x0A,0x31],
    'L': [0x3F,0x20,0x20,0x20],
    'M': [0x3F,0x02,0x04,0x02,0x3F],
    'N': [0x3F,0x02,0x04,0x3F],
    'O': [0x1E,0x21,0x21,0x1E],
    'P': [0x3F,0x05,0x05,0x02],
    'Q': [0x1E,0x21,0x29,0x1E],
    'R': [0x3F,0x05,0x0D,0x32],
    'S': [0x22,0x25,0x25,0x19],
    'T': [0x01,0x01,0x3F,0x01],
    'U': [0x1F,0x20,0x20,0x1F],
    'V': [0x0F,0x10,0x20,0x10,0x0F],
    'W': [0x1F,0x20,0x18,0x20,0x1F],
    'X': [0x31,0x0A,0x04,0x0A,0x31],
    'Y': [0x03,0x04,0x38,0x04,0x03],
    'Z': [0x31,0x29,0x25,0x23],
    'a': [0x10,0x2A,0x2A,0x3C],
    'b': [0x3F,0x24,0x24,0x18],
    'c': [0x1C,0x22,0x22,0x14],
    'd': [0x18,0x24,0x24,0x3F],
    'e': [0x1C,0x2A,0x2A,0x0C],
    'f': [0x04,0x3E,0x05,0x01],
    'g': [0x0C,0x2A,0x2A,0x1C],
    'h': [0x3F,0x04,0x04,0x38],
    'i': [0x00,0x24,0x3D,0x20],
    'j': [0x10,0x20,0x24,0x1D],
    'k': [0x3F,0x08,0x14,0x22],
    'l': [0x00,0x21,0x3F,0x20],
    'm': [0x3C,0x04,0x18,0x04,0x38],
    'n': [0x3C,0x04,0x04,0x38],
    'o': [0x1C,0x22,0x22,0x1C],
    'p': [0x3C,0x14,0x14,0x08],
    'q': [0x08,0x14,0x14,0x3C],
    'r': [0x3C,0x08,0x04,0x04],
    's': [0x24,0x2A,0x2A,0x12],
    't': [0x04,0x1F,0x24,0x20],
    'u': [0x1C,0x20,0x20,0x3C],
    'v': [0x0C,0x10,0x20,0x10,0x0C],
    'w': [0x1C,0x20,0x18,0x20,0x1C],
    'x': [0x22,0x14,0x08,0x14,0x22],
    'y': [0x0C,0x28,0x28,0x1C],
    'z': [0x22,0x32,0x2A,0x26],
    '.': [0x00,0x30,0x30,0x00],
    ':': [0x00,0x12,0x12,0x00],
    '-': [0x08,0x08,0x08,0x08],
    ' ': [0x00,0x00,0x00,0x00],
    '%': [0x23,0x13,0x08,0x64],
    '/': [0x20,0x10,0x08,0x04],
    'V': [0x0F,0x10,0x20,0x10,0x0F],
}

def draw_text(x, y, text, color):
    for ch in text:
        glyph = FONT4.get(ch, FONT4[' '])
        cols = len(glyph)
        for c in range(cols):
            bits = glyph[c]
            for r in range(6):
                if bits & (1 << r):
                    fb.pixel(x + c, y + r, color)
        x += cols + 1   # spasi antar huruf sangat rapat

# ==========================================
# TFT DRIVER
# ==========================================
def command(cmd):
    cs.value(0)
    dc.value(0)
    tft.write(bytes([cmd]))
    cs.value(1)

def data(dat):
    cs.value(0)
    dc.value(1)
    tft.write(dat)
    cs.value(1)

def init_tft():
    rst.value(1); time.sleep_ms(50)
    rst.value(0); time.sleep_ms(50)
    rst.value(1); time.sleep_ms(150)

    command(0x01); time.sleep_ms(150)
    command(0x11); time.sleep_ms(120)

    command(0x3A); data(b'\x05')
    command(0x36); data(b'\x68')

    command(0xB1); data(b'\x05\x3C\x3C')
    command(0xB2); data(b'\x05\x3C\x3C')
    command(0xB3); data(b'\x05\x3C\x3C\x05\x3C\x3C')
    command(0xB4); data(b'\x03')

    command(0xC0); data(b'\x28\x08\x04')
    command(0xC1); data(b'\xC0')
    command(0xC2); data(b'\x0D\x00')
    command(0xC3); data(b'\x8A\x2A')
    command(0xC4); data(b'\x8A\xEE')
    command(0xC5); data(b'\x1A')

    command(0xE0)
    data(bytes([0x04,0x22,0x07,0x0A,0x2E,0x30,0x25,0x2A,
                0x28,0x26,0x2E,0x3A,0x00,0x01,0x03,0x13]))
    command(0xE1)
    data(bytes([0x04,0x16,0x06,0x0D,0x2D,0x26,0x23,0x27,
                0x27,0x25,0x2D,0x3B,0x00,0x01,0x04,0x13]))

    command(0x21)
    command(0x29)
    time.sleep_ms(50)

def show():
    command(0x2A); data(b'\x00\x00\x00\x9F')
    command(0x2B); data(b'\x00\x00\x00\x7F')
    command(0x2C)
    dc.value(1)
    cs.value(0)
    for i in range(0, len(buf), 2):
        buf[i], buf[i+1] = buf[i+1], buf[i]
    tft.write(buf)
    for i in range(0, len(buf), 2):
        buf[i], buf[i+1] = buf[i+1], buf[i]
    cs.value(1)

# ==========================================
# RGB LED
# ==========================================
def set_rgb(r, g, b, br=1.0):
    led_r.duty_u16(int(r*br)*257)
    led_g.duty_u16(int(g*br)*257)
    led_b.duty_u16(int(b*br)*257)

def update_led():
    if mode == 0:   r,g,b = 0,200,255
    elif mode == 1: r,g,b = 255,180,0
    else:           r,g,b = 255,0,160
    set_rgb(r, g, b, 0.8 if running else 0.1)

# ==========================================
# STATE
# ==========================================
mode = 0
freq = 5.0
running = True
phase_offset = 0.0

X0, Y0 = 0, 24
GW, GH = 160, 74
CENTER_Y = Y0 + GH // 2
AMPLITUDE = 32

meas = {"max":0,"min":0,"vpp":0,"avg":0,"rms":0,"dty":50,"frq":0,"per":0}

def waveform_value(x, phase=0.0):
    angle = (x / (GW-1)) * 2 * math.pi * freq + phase_offset + phase
    if mode == 0:
        return math.sin(angle)
    elif mode == 1:
        return 1.0 if math.sin(angle) >= 0 else -1.0
    else:
        p = (angle / (2*math.pi)) % 1.0
        return 4*abs(p-0.5) - 1

def to_volt(val):
    return 1.65 + val * 1.65

def calculate_measurements():
    samples = []
    high = 0
    for x in range(0, GW, 2):
        v = to_volt(waveform_value(x))
        samples.append(v)
        if v >= 1.65: high += 1

    meas["max"] = max(samples)
    meas["min"] = min(samples)
    meas["vpp"] = meas["max"] - meas["min"]
    meas["avg"] = sum(samples) / len(samples)
    meas["rms"] = math.sqrt(sum(s*s for s in samples) / len(samples))
    meas["dty"] = (high / len(samples)) * 100
    meas["frq"] = freq
    meas["per"] = 1000.0 / freq if freq > 0 else 0

# ==========================================
# DRAWING - Dark Mode + Font kecil
# ==========================================
def draw_top():
    fb.fill_rect(0, 0, W, 23, BLACK)

    draw_text(2,  2, "Max:{:.2f}V".format(meas["max"]), WHITE)
    draw_text(2, 10, "Min:{:.2f}V".format(meas["min"]), WHITE)
    draw_text(2, 18, "Frq:{:.2f}Hz".format(meas["frq"]), CYAN)

    draw_text(88,  2, "Vpp:{:.2f}V".format(meas["vpp"]), WHITE)
    draw_text(88, 10, "Avg:{:.2f}V".format(meas["avg"]), WHITE)
    draw_text(88, 18, "Per:{:.1f}ms".format(meas["per"]), CYAN)

def draw_grid():
    fb.fill_rect(X0, Y0, GW, GH, BLACK)

    for x in range(X0, X0+GW, 32):
        fb.vline(x, Y0, GH, DARK)
    for y in range(Y0, Y0+GH, 18):
        fb.hline(X0, y, GW, DARK)

    fb.hline(X0, CENTER_Y, GW, 0x03E0)          # garis tengah hijau gelap
    fb.vline(X0 + GW//2, Y0, GH, 0x4208)        # garis vertikal

def draw_wave():
    prev = CENTER_Y
    for x in range(GW):
        val = waveform_value(x)
        y = int(CENTER_Y - val * AMPLITUDE)
        y = max(Y0, min(Y0+GH-1, y))
        if x > 0:
            fb.line(X0+x-1, prev, X0+x, y, WAVE)
        prev = y

def draw_bottom():
    fb.fill_rect(0, 100, W, 28, BLACK)
    fb.hline(0, 99, W, DARK)

    draw_text(2, 103, "Dty:{:.1f}%".format(meas["dty"]), CYAN)
    draw_text(2, 113, "1V/div", GRAY)

    if running:
        fb.fill_rect(66, 106, 28, 11, GREEN)
        draw_text(71, 108, "RUN", BLACK)
    else:
        fb.fill_rect(66, 106, 28, 11, RED)
        draw_text(68, 108, "HOLD", WHITE)

    draw_text(104, 103, "Rms:{:.2f}V".format(meas["rms"]), YELLOW)
    draw_text(104, 113, "20ms/div", GRAY)

def render():
    calculate_measurements()
    draw_top()
    draw_grid()
    draw_wave()
    draw_bottom()
    show()
    update_led()

# ==========================================
# INPUT
# ==========================================
last_clk = clk.value()
last_enc = enc_sw.value()
last_btn = button.value()
last_enc_time = time.ticks_ms()
last_btn_time = time.ticks_ms()
last_rot_time = time.ticks_ms()

def read_controls():
    global last_clk, last_enc, last_btn
    global last_enc_time, last_btn_time, last_rot_time
    global freq, mode, running

    now = time.ticks_ms()

    current_clk = clk.value()
    if current_clk != last_clk and current_clk == 0:
        if time.ticks_diff(now, last_rot_time) > 25:
            if dt.value() != current_clk:
                freq = min(10.0, round(freq + 0.5, 1))
            else:
                freq = max(0.5, round(freq - 0.5, 1))
            last_rot_time = now
    last_clk = current_clk

    current_enc = enc_sw.value()
    if current_enc == 0 and last_enc == 1:
        if time.ticks_diff(now, last_enc_time) > 200:
            mode = (mode + 1) % 3
            last_enc_time = now
    last_enc = current_enc

    current_btn = button.value()
    if current_btn == 0 and last_btn == 1:
        if time.ticks_diff(now, last_btn_time) > 200:
            running = not running
            last_btn_time = now
    last_btn = current_btn

# ==========================================
# MAIN
# ==========================================
init_tft()
render()

while True:
    read_controls()
    if running:
        phase_offset += 0.22
    render()
    time.sleep_ms(26)
