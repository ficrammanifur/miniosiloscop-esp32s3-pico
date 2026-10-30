from machine import Pin, SPI, PWM, ADC
import framebuf
import time
import math

# ==========================================
# RASPBERRY PI PICO - REAL MINI OSCILLOSCOPE
# Gelombang = data ADC asli
# Time/Div & V/Div benar-benar berfungsi
# ==========================================
W = 160
H = 128

tft = SPI(0, baudrate=30_000_000, polarity=0, phase=0,
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

adc = ADC(26)

buf = bytearray(W * H * 2)
fb  = framebuf.FrameBuffer(buf, W, H, framebuf.RGB565)
display_buf = bytearray(W * H * 2)

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
# FONT 4x6
# ==========================================
FONT4 = {
    '0':[0x1E,0x29,0x25,0x1E],'1':[0x00,0x22,0x3F,0x20],'2':[0x32,0x29,0x25,0x22],
    '3':[0x11,0x21,0x25,0x1B],'4':[0x0C,0x0A,0x3F,0x08],'5':[0x17,0x25,0x25,0x19],
    '6':[0x1E,0x25,0x25,0x18],'7':[0x01,0x39,0x05,0x03],'8':[0x1A,0x25,0x25,0x1A],
    '9':[0x06,0x25,0x25,0x1E],'A':[0x3E,0x09,0x09,0x3E],'B':[0x3F,0x25,0x25,0x1A],
    'C':[0x1E,0x21,0x21,0x12],'D':[0x3F,0x21,0x21,0x1E],'E':[0x3F,0x25,0x25,0x21],
    'F':[0x3F,0x05,0x05,0x01],'G':[0x1E,0x21,0x25,0x1D],'H':[0x3F,0x04,0x04,0x3F],
    'I':[0x00,0x21,0x3F,0x21],'J':[0x10,0x20,0x21,0x1F],'K':[0x3F,0x04,0x0A,0x31],
    'L':[0x3F,0x20,0x20,0x20],'M':[0x3F,0x02,0x04,0x02,0x3F],'N':[0x3F,0x02,0x04,0x3F],
    'O':[0x1E,0x21,0x21,0x1E],'P':[0x3F,0x05,0x05,0x02],'Q':[0x1E,0x21,0x29,0x1E],
    'R':[0x3F,0x05,0x0D,0x32],'S':[0x22,0x25,0x25,0x19],'T':[0x01,0x01,0x3F,0x01],
    'U':[0x1F,0x20,0x20,0x1F],'V':[0x0F,0x10,0x20,0x10,0x0F],'W':[0x1F,0x20,0x18,0x20,0x1F],
    'X':[0x31,0x0A,0x04,0x0A,0x31],'Y':[0x03,0x04,0x38,0x04,0x03],'Z':[0x31,0x29,0x25,0x23],
    'a':[0x10,0x2A,0x2A,0x3C],'b':[0x3F,0x24,0x24,0x18],'c':[0x1C,0x22,0x22,0x14],
    'd':[0x18,0x24,0x24,0x3F],'e':[0x1C,0x2A,0x2A,0x0C],'f':[0x04,0x3E,0x05,0x01],
    'g':[0x0C,0x2A,0x2A,0x1C],'h':[0x3F,0x04,0x04,0x38],'i':[0x00,0x24,0x3D,0x20],
    'j':[0x10,0x20,0x24,0x1D],'k':[0x3F,0x08,0x14,0x22],'l':[0x00,0x21,0x3F,0x20],
    'm':[0x3C,0x04,0x18,0x04,0x38],'n':[0x3C,0x04,0x04,0x38],'o':[0x1C,0x22,0x22,0x1C],
    'p':[0x3C,0x14,0x14,0x08],'q':[0x08,0x14,0x14,0x3C],'r':[0x3C,0x08,0x04,0x04],
    's':[0x24,0x2A,0x2A,0x12],'t':[0x04,0x1F,0x24,0x20],'u':[0x1C,0x20,0x20,0x3C],
    'v':[0x0C,0x10,0x20,0x10,0x0C],'w':[0x1C,0x20,0x18,0x20,0x1C],'x':[0x22,0x14,0x08,0x14,0x22],
    'y':[0x0C,0x28,0x28,0x1C],'z':[0x22,0x32,0x2A,0x26],
    '.':[0x00,0x30,0x30,0x00],':':[0x00,0x12,0x12,0x00],'-':[0x08,0x08,0x08,0x08],
    ' ':[0x00,0x00,0x00,0x00],'%':[0x23,0x13,0x08,0x64],'/':[0x20,0x10,0x08,0x04],
}

def draw_text(x, y, text, color):
    for ch in text:
        glyph = FONT4.get(ch, FONT4[' '])
        for c, bits in enumerate(glyph):
            for r in range(6):
                if bits & (1 << r):
                    fb.pixel(x + c, y + r, color)
        x += len(glyph) + 1

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
    data(bytes([0x04,0x22,0x07,0x0A,0x2E,0x30,0x25,0x2A,0x28,0x26,0x2E,0x3A,0x00,0x01,0x03,0x13]))
    command(0xE1)
    data(bytes([0x04,0x16,0x06,0x0D,0x2D,0x26,0x23,0x27,0x27,0x25,0x2D,0x3B,0x00,0x01,0x04,0x13]))
    command(0x21)
    command(0x29)
    time.sleep_ms(40)

def show():
    command(0x2A)
    data(b'\x00\x00\x00\x9F')
    command(0x2B)
    data(b'\x00\x00\x00\x7F')
    command(0x2C)

    for i in range(0, len(buf), 2):
        display_buf[i]     = buf[i + 1]
        display_buf[i + 1] = buf[i]

    dc.value(1)
    cs.value(0)
    tft.write(display_buf)
    cs.value(1)

# ==========================================
# RGB LED
# ==========================================
def set_rgb(r, g, b, br=1.0):
    led_r.duty_u16(int(r * br) * 257)
    led_g.duty_u16(int(g * br) * 257)
    led_b.duty_u16(int(b * br) * 257)

def update_led():
    if running:
        set_rgb(0, 220, 80, 0.75)
    else:
        set_rgb(180, 40, 0, 0.35)

# ==========================================
# SETTINGS
# ==========================================
running = True

# Parameter yang sedang diedit: 0 = Time/Div, 1 = V/Div
edit_param = 0

# Time/Div (ms per divisi horizontal)
# Grid: 5 div horizontal (160px / 32px)
time_div_list = [0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0]  # ms/div
td_index = 3          # default 1.0 ms/div
time_div_ms = time_div_list[td_index]

# V/Div
v_div_list = [0.1, 0.2, 0.5, 1.0, 2.0]
vd_index = 2
v_div = v_div_list[vd_index]

SAMPLES = 160
samples = [0.0] * SAMPLES
capture_time_s = 0.0

meas = {
    "max": 0.0, "min": 0.0, "vpp": 0.0,
    "avg": 0.0, "rms": 0.0, "dty": 50.0,
    "frq": 0.0, "per": 0.0
}

X0, Y0 = 0, 24
GW, GH = 160, 74
CENTER_Y = Y0 + GH // 2
PX_PER_DIV_V = 18     # vertikal
PX_PER_DIV_H = 32     # horizontal
H_DIVS = 5            # 160 / 32 = 5 div

# ==========================================
# ADC SAMPLING (tanpa read_controls di dalam loop)
# ==========================================
def sample_adc():
    global samples, capture_time_s

    # Hitung interval sampling dari Time/Div
    # Total waktu di layar = H_DIVS * time_div_ms (ms)
    total_time_ms = H_DIVS * time_div_ms
    interval_us = int((total_time_ms * 1000) / SAMPLES)
    if interval_us < 5:
        interval_us = 5

    raw = []
    start = time.ticks_us()

    for _ in range(SAMPLES):
        val = adc.read_u16()
        raw.append((val / 65535.0) * 3.3)
        if interval_us > 0:
            time.sleep_us(interval_us)

    end = time.ticks_us()
    capture_time_s = time.ticks_diff(end, start) / 1_000_000.0
    samples = raw

def calculate_measurements():
    if not samples:
        return

    mn = min(samples)
    mx = max(samples)
    avg = sum(samples) / len(samples)

    sq = sum(s * s for s in samples) / len(samples)
    rms = math.sqrt(sq)

    high = sum(1 for s in samples if s >= avg)
    dty = (high / len(samples)) * 100.0

    # Frekuensi dari rising edge
    mid = (mn + mx) / 2.0
    rising = 0
    for i in range(1, len(samples)):
        if samples[i-1] < mid and samples[i] >= mid:
            rising += 1

    total_s = capture_time_s if capture_time_s > 0 else 0.001

    if rising > 0 and total_s > 0:
        freq = rising / total_s
        period_ms = 1000.0 / freq
    else:
        freq = 0.0
        period_ms = 0.0

    meas["max"] = mx
    meas["min"] = mn
    meas["vpp"] = mx - mn
    meas["avg"] = avg
    meas["rms"] = rms
    meas["dty"] = dty
    meas["frq"] = freq
    meas["per"] = period_ms

# ==========================================
# DRAWING
# ==========================================
def draw_top():
    fb.fill_rect(0, 0, W, 23, BLACK)
    draw_text(2,  2, "Max:{:.2f}V".format(meas["max"]), WHITE)
    draw_text(2, 10, "Min:{:.2f}V".format(meas["min"]), WHITE)
    draw_text(2, 18, "Frq:{:.1f}Hz".format(meas["frq"]), CYAN)

    draw_text(88,  2, "Vpp:{:.2f}V".format(meas["vpp"]), WHITE)
    draw_text(88, 10, "Avg:{:.2f}V".format(meas["avg"]), WHITE)
    draw_text(88, 18, "Per:{:.1f}ms".format(meas["per"]), CYAN)

def draw_grid():
    fb.fill_rect(X0, Y0, GW, GH, BLACK)
    for x in range(X0, X0 + GW, PX_PER_DIV_H):
        fb.vline(x, Y0, GH, DARK)
    for y in range(Y0, Y0 + GH, PX_PER_DIV_V):
        fb.hline(X0, y, GW, DARK)
    fb.hline(X0, CENTER_Y, GW, 0x03E0)
    fb.vline(X0 + GW // 2, Y0, GH, 0x4208)

def draw_wave():
    """Gambar gelombang dari data ADC asli"""
    if not samples:
        return

    # V/div yang benar: 1 div = PX_PER_DIV_V pixel
    pixels_per_volt = PX_PER_DIV_V / max(v_div, 0.05)
    center_v = meas["avg"]          # auto-center ke rata-rata

    prev_y = None
    for i, v in enumerate(samples):
        y = int(CENTER_Y - (v - center_v) * pixels_per_volt)
        y = max(Y0 + 1, min(Y0 + GH - 2, y))

        if prev_y is not None:
            fb.line(X0 + i - 1, prev_y, X0 + i, y, WAVE)
        prev_y = y

def draw_bottom():
    fb.fill_rect(0, 100, W, 28, BLACK)
    fb.hline(0, 99, W, DARK)

    # Kiri: Duty + V/div
    draw_text(2, 103, "Dty:{:.0f}%".format(meas["dty"]), CYAN)
    # Tampilkan V/div, beri tanda jika sedang diedit
    if edit_param == 1:
        draw_text(2, 113, ">{:.1f}V/div".format(v_div), YELLOW)
    else:
        draw_text(2, 113, "{:.1f}V/div".format(v_div), GRAY)

    # Tengah: RUN/HOLD
    if running:
        fb.fill_rect(66, 102, 28, 11, GREEN)
        draw_text(71, 104, "RUN", BLACK)
    else:
        fb.fill_rect(66, 102, 28, 11, RED)
        draw_text(68, 104, "HOLD", WHITE)

    # Mode edit di bawah RUN
    if edit_param == 0:
        draw_text(64, 115, "T/DIV", YELLOW)
    else:
        draw_text(64, 115, "V/DIV", YELLOW)

    # Kanan: RMS + Time/Div
    draw_text(100, 103, "Rms:{:.2f}V".format(meas["rms"]), YELLOW)
    if edit_param == 0:
        draw_text(100, 113, ">{:.1f}ms".format(time_div_ms), YELLOW)
    else:
        draw_text(100, 113, "{:.1f}ms/div".format(time_div_ms), GRAY)

def render():
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
# Variabel tambahan di bagian global (dekat last_clk)
enc_last_state = (clk.value() << 1) | dt.value()

def read_controls():
    global last_clk, last_enc, last_btn
    global last_enc_time, last_btn_time, last_rot_time
    global time_div_ms, td_index, v_div, vd_index
    global running, edit_param, enc_last_state

    now = time.ticks_ms()

    # ===== Encoder sangat sensitif (setiap perubahan state dihitung) =====
    state = (clk.value() << 1) | dt.value()

    if state != enc_last_state:
        # Tabel transisi kuadratur sederhana
        # 00→01→11→10→00 = satu arah
        # 00→10→11→01→00 = arah sebaliknya
        transition = (enc_last_state << 2) | state

        direction = 0
        if transition in (0b0001, 0b0111, 0b1110, 0b1000):
            direction = 1          # putar kanan
        elif transition in (0b0010, 0b1011, 0b1101, 0b0100):
            direction = -1         # putar kiri

        if direction != 0 and time.ticks_diff(now, last_rot_time) > 0:
            if edit_param == 0:
                td_index = max(0, min(len(time_div_list) - 1, td_index + direction))
                time_div_ms = time_div_list[td_index]
            else:
                vd_index = max(0, min(len(v_div_list) - 1, vd_index + direction))
                v_div = v_div_list[vd_index]
            last_rot_time = now

        enc_last_state = state

    # ===== Tekan encoder → ganti Time/Div ↔ V/Div =====
    current_enc = enc_sw.value()
    if current_enc == 0 and last_enc == 1:
        if time.ticks_diff(now, last_enc_time) > 150:
            edit_param = 1 - edit_param
            last_enc_time = now
    last_enc = current_enc

    # ===== Tombol → RUN / HOLD =====
    current_btn = button.value()
    if current_btn == 0 and last_btn == 1:
        if time.ticks_diff(now, last_btn_time) > 150:
            running = not running
            last_btn_time = now
    last_btn = current_btn

# ==========================================
# MAIN
# ==========================================
init_tft()

sample_adc()
calculate_measurements()
render()

while True:
    read_controls()          # sebelum sampling

    if running:
        sample_adc()
        calculate_measurements()

    read_controls()          # sesudah sampling
    render()
    read_controls()          # sesudah render
