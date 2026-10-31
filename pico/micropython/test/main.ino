from machine import Pin, SPI, PWM, ADC, WDT, disable_irq, enable_irq
import machine
import framebuf
import time
import math
import sys
from array import array

machine.freq(200_000_000)   # overclock (hapus kalau tidak mau)

# ================= OPSI =================
INVERT     = False   # False = background hitam (coba True kalau warna terbalik)
ENC_DIV    = 2       # 1 = super sensitif, 2 = sedang, 4 = 1 klik 1 langkah
ENC_REV    = False   # True kalau arah putar terbalik
SMOOTH     = False   #True    haluskan gelombang (moving average 3 titik)
THICK      = True    # garis gelombang lebih tebal
USE_WDT    = False   # True = watchdog aktif (nyalakan setelah program stabil)
AUTO_RESET = False   # True = restart otomatis kalau error (nyalakan setelah stabil)
# ========================================

W = 160
H = 128

def c(v):   # tukar byte warna sekali saja -> tanpa loop swap di show()
    return ((v & 0xFF) << 8) | (v >> 8)

BLACK  = c(0x0000)
WHITE  = c(0xFFFF)
CYAN   = c(0x07FF)
GREEN  = c(0x07E0)
YELLOW = c(0xFFE0)
RED    = c(0xF800)
GRAY   = c(0x7BEF)
DARK   = c(0x2104)
WAVE   = c(0x07FF)
G_CTR  = c(0x03E0)
G_MID  = c(0x4208)

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

# ================= FONT 4x6 =================
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
    '>':[0x00,0x22,0x14,0x08],
}

def draw_text(x, y, text, color):
    for ch in text:
        glyph = FONT4.get(ch, FONT4[' '])
        for col, bits in enumerate(glyph):
            for r in range(6):
                if bits & (1 << r):
                    fb.pixel(x + col, y + r, color)
        x += len(glyph) + 1

# ================= TFT DRIVER =================
def command(cmd):
    cs.value(0); dc.value(0)
    tft.write(bytes([cmd]))
    cs.value(1)

def data(dat):
    cs.value(0); dc.value(1)
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
    command(0x21 if INVERT else 0x20)
    command(0x29)
    time.sleep_ms(40)

def show():
    command(0x2A); data(b'\x00\x00\x00\x9F')
    command(0x2B); data(b'\x00\x00\x00\x7F')
    command(0x2C)
    dc.value(1); cs.value(0)
    tft.write(buf)
    cs.value(1)

# ================= LED =================
def set_rgb(r, g, b, br=1.0):
    led_r.duty_u16(int(r * br) * 257)
    led_g.duty_u16(int(g * br) * 257)
    led_b.duty_u16(int(b * br) * 257)

def update_led():
    if running: set_rgb(0, 220, 80, 0.75)
    else:       set_rgb(180, 40, 0, 0.35)

# ================= SETTINGS =================
running    = True
edit_param = 0
ui_dirty   = True
frame      = 0

time_div_list = [0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0]
td_index = 3
time_div_ms = time_div_list[td_index]

v_div_list = [0.05, 0.1, 0.2, 0.5, 1.0, 2.0]
vd_index = 3
v_div = v_div_list[vd_index]

SAMPLES = 160
EXTRA   = 80
BUF_N   = SAMPLES + EXTRA
raw     = array('H', [0] * BUF_N)
samples = [0.0] * SAMPLES
window_s = 0.001

meas = {"max":0.0,"min":0.0,"vpp":0.0,"avg":0.0,"rms":0.0,"dty":50.0,"frq":0.0,"per":0.0}

X0, Y0 = 0, 24
GW, GH = 160, 74
CENTER_Y = Y0 + GH // 2
PX_PER_DIV_V = 18
PX_PER_DIV_H = 32
H_DIVS = 5

# ================= SAMPLING + TRIGGER =================
def sample_adc():
    global samples, window_s

    total_time_ms = H_DIVS * time_div_ms
    interval_us = int((total_time_ms * 1000) / SAMPLES)
    read = adc.read_u16
    sleep = time.sleep_us

    t0 = time.ticks_us()
    if interval_us > 20:
        for i in range(BUF_N):
            raw[i] = read()
            sleep(interval_us - 15)
    else:
        for i in range(BUF_N):
            raw[i] = read()
    t1 = time.ticks_us()

    per_sample = time.ticks_diff(t1, t0) / 1_000_000.0 / BUF_N
    window_s = per_sample * SAMPLES

    mn = min(raw); mx = max(raw)
    mid = (mn + mx) >> 1
    start = 0
    for i in range(1, EXTRA):
        if raw[i-1] < mid <= raw[i]:
            start = i
            break

    k = 3.3 / 65535.0
    v = [raw[start + i] * k for i in range(SAMPLES)]

    if SMOOTH:
        s = [v[0]]
        for i in range(1, SAMPLES - 1):
            s.append((v[i-1] + 2 * v[i] + v[i+1]) * 0.25)
        s.append(v[-1])
        v = s
    samples = v

def calculate_measurements():
    mn = min(samples); mx = max(samples)
    avg = sum(samples) / SAMPLES
    rms = math.sqrt(sum(s * s for s in samples) / SAMPLES)
    high = sum(1 for s in samples if s >= avg)

    mid = (mn + mx) / 2.0
    rising = 0
    for i in range(1, SAMPLES):
        if samples[i-1] < mid <= samples[i]:
            rising += 1

    if rising > 0:
        freq = rising / window_s
        per = 1000.0 / freq
    else:
        freq = 0.0; per = 0.0

    meas["max"] = mx; meas["min"] = mn; meas["vpp"] = mx - mn
    meas["avg"] = avg; meas["rms"] = rms
    meas["dty"] = high * 100.0 / SAMPLES
    meas["frq"] = freq; meas["per"] = per

# ================= DRAWING =================
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
    fb.hline(X0, CENTER_Y, GW, G_CTR)
    fb.vline(X0 + GW // 2, Y0, GH, G_MID)

def draw_wave():
    ppv = PX_PER_DIV_V / v_div
    cv = meas["avg"]
    ymin = Y0 + 1
    ymax = Y0 + GH - 3
    prev_y = None
    for i in range(SAMPLES):
        y = int(CENTER_Y - (samples[i] - cv) * ppv)
        if y < ymin: y = ymin
        elif y > ymax: y = ymax
        if prev_y is not None:
            fb.line(X0 + i - 1, prev_y, X0 + i, y, WAVE)
            if THICK:
                fb.line(X0 + i - 1, prev_y + 1, X0 + i, y + 1, WAVE)
        prev_y = y

def draw_bottom():
    fb.fill_rect(0, 100, W, 28, BLACK)
    fb.hline(0, 99, W, DARK)
    draw_text(2, 103, "Dty:{:.0f}%".format(meas["dty"]), CYAN)
    if edit_param == 1: draw_text(2, 113, ">{:.2f}V/div".format(v_div), YELLOW)
    else:               draw_text(2, 113, "{:.2f}V/div".format(v_div), GRAY)

    if running:
        fb.fill_rect(66, 102, 28, 11, GREEN)
        draw_text(71, 104, "RUN", BLACK)
    else:
        fb.fill_rect(66, 102, 28, 11, RED)
        draw_text(68, 104, "HOLD", WHITE)
    draw_text(64, 115, "T/DIV" if edit_param == 0 else "V/DIV", YELLOW)

    draw_text(100, 103, "Rms:{:.2f}V".format(meas["rms"]), YELLOW)
    if edit_param == 0: draw_text(100, 113, ">{:.1f}ms".format(time_div_ms), YELLOW)
    else:               draw_text(100, 113, "{:.1f}ms/div".format(time_div_ms), GRAY)

def render():
    global frame, ui_dirty
    frame += 1
    if ui_dirty or frame % 4 == 0:
        draw_top()
        draw_bottom()
        ui_dirty = False
    draw_grid()
    draw_wave()
    show()

# ================= INPUT (INTERRUPT) =================
ENC_TABLE = (0,1,-1,0, -1,0,0,1, 1,0,0,-1, 0,-1,1,0)
enc_state = (clk.value() << 1) | dt.value()
enc_count = 0
btn_flag = False
esw_flag = False
last_btn_t = time.ticks_ms()
last_esw_t = time.ticks_ms()

def enc_isr(pin):
    global enc_state, enc_count
    s = (clk.value() << 1) | dt.value()
    enc_count += ENC_TABLE[(enc_state << 2) | s]
    enc_state = s

def btn_isr(pin):
    global btn_flag, last_btn_t
    now = time.ticks_ms()
    if time.ticks_diff(now, last_btn_t) > 200:
        btn_flag = True
        last_btn_t = now

def esw_isr(pin):
    global esw_flag, last_esw_t
    now = time.ticks_ms()
    if time.ticks_diff(now, last_esw_t) > 200:
        esw_flag = True
        last_esw_t = now

both = Pin.IRQ_FALLING | Pin.IRQ_RISING
clk.irq(trigger=both, handler=enc_isr)
dt.irq(trigger=both, handler=enc_isr)
button.irq(trigger=Pin.IRQ_FALLING, handler=btn_isr)
enc_sw.irq(trigger=Pin.IRQ_FALLING, handler=esw_isr)

def handle_inputs():
    global enc_count, btn_flag, esw_flag
    global td_index, time_div_ms, vd_index, v_div
    global running, edit_param, ui_dirty

    irq = disable_irq()
    n = enc_count
    steps = int(n / ENC_DIV)
    enc_count -= steps * ENC_DIV
    b = btn_flag; btn_flag = False
    e = esw_flag; esw_flag = False
    enable_irq(irq)

    if steps:
        if ENC_REV: steps = -steps
        if edit_param == 0:
            td_index = max(0, min(len(time_div_list) - 1, td_index + steps))
            time_div_ms = time_div_list[td_index]
        else:
            vd_index = max(0, min(len(v_div_list) - 1, vd_index + steps))
            v_div = v_div_list[vd_index]
        ui_dirty = True

    if e:
        edit_param = 1 - edit_param
        ui_dirty = True
    if b:
        running = not running
        update_led()
        ui_dirty = True

# ================= MAIN (HARUS PALING BAWAH) =================
def main():
    global running
    running = True                      # selalu mulai dalam mode RUN

    init_tft()
    update_led()
    sample_adc()
    calculate_measurements()
    render()

    wdt = WDT(timeout=5000) if USE_WDT else None

    while True:
        handle_inputs()
        if running:
            sample_adc()
            calculate_measurements()
        handle_inputs()
        render()
        if wdt:
            wdt.feed()

try:
    main()
except Exception as e:
    sys.print_exception(e)              # tampilkan error lengkap di Thonny
    if AUTO_RESET:
        time.sleep_ms(500)
        machine.reset()
