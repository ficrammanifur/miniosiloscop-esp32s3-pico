# ==========================================================================
#  RASPBERRY PI PICO 2 (RP2350) - DMA OSCILLOSCOPE
#
#  * Sampling ADC via DMA : sampai 500 ksps (1 channel) / 250 ksps per channel (2 channel)
#  * 2 channel            : CH1 = GP26 (ADC0), CH2 = GP27 (ADC1)
#  * Trigger beneran      : level, rising/falling, AUTO / NORM / SINGLE, ada pre-trigger
#  * Tampilan             : envelope min/max per kolom (riak PWM kelihatan jelas)
#  * Pengukuran akurat    : frekuensi interpolasi, Vpp, Avg, Rms, Max, Min, Period, Duty, Gain
#  * AUTOSET              : atur T/div, V/div, trigger otomatis
#
#  KONTROL
#    Putar encoder          : ubah parameter yang terpilih (digaris bawah kuning)
#    Tekan encoder          : pindah parameter  T/DIV > V/DIV > LEVEL > EDGE > MODE > CH > CTR
#    Tahan encoder (0.7 dtk): AUTOSET
#    Tombol (tekan)         : RUN / HOLD   (di HOLD, V/div, Ctr dan T/div tetap bisa diubah / zoom)
#    Tombol (tahan 0.7 dtk) : ganti halaman pengukuran
#
#  Simpan ke Pico 2 dengan nama  main.py
#  (firmware MicroPython harus versi Pico 2 / RP2350)
# ==========================================================================
from machine import Pin, SPI, PWM, ADC, WDT, mem32, disable_irq, enable_irq
import machine
import framebuf
import time
import math
import sys
import os
import micropython
import rp2
from array import array

# ============================ OPSI ========================================
CPU_MHZ    = 200          # 150 = standar, 200 = overclock (turunkan kalau tidak stabil)
SPI_HZ     = 30_000_000   # kecepatan SPI ke TFT
INVERT     = False        # False = background hitam
ENC_DIV    = 2            # 1 = super sensitif, 2 = sedang, 4 = 1 klik 1 langkah
ENC_REV    = False        # True kalau arah putar terbalik
USE_DMA    = True         # False = paksa mode software (lambat, hanya untuk uji)
USE_WDT    = False        # True = watchdog (nyalakan setelah stabil)
AUTO_RESET = False        # True = restart otomatis kalau error (nyalakan setelah stabil)
LONG_MS    = 700          # lama tahan untuk aksi "long press"
TRIG_HYST  = 25           # histeresis trigger (satuan count ADC, 25 = sekitar 20 mV)
# ==========================================================================

machine.freq(CPU_MHZ * 1_000_000)

try:
    IS_2350 = "2350" in os.uname().machine
except Exception:
    IS_2350 = True

W = 160
H = 128
X0, Y0 = 0, 24
GW, GH = 160, 74
CENTER_Y = Y0 + GH // 2
PX_PER_DIV_V = 18
PX_PER_DIV_H = 32
H_DIVS = 5
TRIG_X = 32                       # posisi titik trigger di layar (1 div dari kiri)
GS = Y0 * W * 2                   # awal & akhir area grid di framebuffer
GE = (Y0 + GH) * W * 2

VREF   = 3.3
ADC_CLK = 48_000_000
NS_MAX = 1024                     # sampel maksimum per channel

# ============================ WARNA =======================================
def c(v):                         # tukar byte sekali saja -> buffer langsung dikirim ke TFT
    return ((v & 0xFF) << 8) | (v >> 8)

BLACK   = c(0x0000)
WHITE   = c(0xFFFF)
CYAN    = c(0x07FF)
MAGENTA = c(0xF81F)
GREEN   = c(0x07E0)
YELLOW  = c(0xFFE0)
RED     = c(0xF800)
GRAY    = c(0x7BEF)
DARK    = c(0x2104)
G_CTR   = c(0x03E0)
G_MID   = c(0x4208)
CH_COL  = (CYAN, MAGENTA)
KEY     = 0x0001                  # warna transparan untuk glyph (tidak dipakai warna lain)

# ============================ HARDWARE ====================================
tft = SPI(0, baudrate=SPI_HZ, polarity=0, phase=0,
          sck=Pin(18), mosi=Pin(19), miso=None)
dc  = Pin(16, Pin.OUT)
cs  = Pin(17, Pin.OUT, value=1)
rst = Pin(20, Pin.OUT)

clk    = Pin(10, Pin.IN, Pin.PULL_UP)
dt_pin = Pin(11, Pin.IN, Pin.PULL_UP)
enc_sw = Pin(12, Pin.IN, Pin.PULL_UP)
button = Pin(13, Pin.IN, Pin.PULL_UP)

led_r = PWM(Pin(2)); led_r.freq(1000)
led_g = PWM(Pin(3)); led_g.freq(1000)
led_b = PWM(Pin(4)); led_b.freq(1000)

adc0 = ADC(Pin(26))               # CH1
adc1 = ADC(Pin(27))               # CH2

buf = bytearray(W * H * 2)
fb  = framebuf.FrameBuffer(buf, W, H, framebuf.RGB565)
mv  = memoryview(buf)
grid_img = None

# ============================ FONT 4x6 ====================================
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
    '+':[0x04,0x0E,0x04],
}

_gcache = {}

def glyph(ch, color):
    k = (ch, color)
    g = _gcache.get(k)
    if g is None:
        cols_ = FONT4.get(ch, FONT4[' '])
        gw = len(cols_)
        ba = bytearray(gw * 6 * 2)
        gf = framebuf.FrameBuffer(ba, gw, 6, framebuf.RGB565)
        gf.fill(KEY)
        for ci, bits in enumerate(cols_):
            for r in range(6):
                if bits & (1 << r):
                    gf.pixel(ci, r, color)
        g = (gf, gw, ba)
        _gcache[k] = g
    return g

def draw_text(x, y, text, color):          # 1 blit per huruf (jauh lebih cepat dari per-pixel)
    for ch in text:
        g = glyph(ch, color)
        fb.blit(g[0], x, y, KEY)
        x += g[1] + 1
    return x

def text_w(text):
    w = 0
    for ch in text:
        w += len(FONT4.get(ch, FONT4[' '])) + 1
    return w - 1

# ============================ TFT DRIVER ==================================
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

# ============================ LED =========================================
def set_rgb(r, g, b, br=1.0):
    led_r.duty_u16(int(r * br) * 257)
    led_g.duty_u16(int(g * br) * 257)
    led_b.duty_u16(int(b * br) * 257)

def update_led():
    if status == 'RUN':
        set_rgb(0, 220, 80, 0.75)
    elif status == 'WAIT':
        set_rgb(255, 140, 0, 0.5)
    else:
        set_rgb(180, 40, 0, 0.35)

# ============================ ADC + DMA ===================================
ADC_BASE = 0x400A0000 if IS_2350 else 0x4004C000
ADC_CS   = ADC_BASE + 0x00
ADC_FCS  = ADC_BASE + 0x08
ADC_FIFO = ADC_BASE + 0x0C
ADC_DIV  = ADC_BASE + 0x10
DREQ_ADC = 48 if IS_2350 else 36

CS_EN         = 1
CS_START_MANY = 1 << 3
CS_READY      = 1 << 8
FCS_EN        = 1
FCS_DREQ      = 1 << 3
FCS_CLR       = (1 << 10) | (1 << 11)

dma = None
DMA_CTRL = 0
if USE_DMA:
    try:
        dma = rp2.DMA()
        DMA_CTRL = dma.pack_ctrl(size=1, inc_read=False, inc_write=True, treq_sel=DREQ_ADC)
    except Exception as e:
        print("DMA init gagal, pakai mode software:", e)
        USE_DMA = False

bufA = array('H', [0] * (2 * NS_MAX))
bufB = array('H', [0] * (2 * NS_MAX))
cap_buf = bufA                    # buffer yang sedang diisi
cur_buf = bufB                    # buffer hasil capture terakhir (dipakai untuk tampilan)
prm  = array('i', [0] * 16)       # parameter untuk fungsi viper
res  = array('i', [0] * 16)       # hasil dari fungsi viper
cols = array('H', [0] * 1280)     # envelope per kolom: tiap channel 4 x 160

# --- slot parameter ---
# prm: 0 base, 1 step, 2 a, 3 b, 4 mid/level, 5 lo/hyst, 6 rising, 7 n,
#      8 s0 (16.16), 9 step (16.16), 10 offset kolom

@micropython.viper
def find_trigger(buf: ptr16, prm: ptr32) -> int:
    base = int(prm[0])
    step = int(prm[1])
    i0 = int(prm[2])
    i1 = int(prm[3])
    level = int(prm[4])
    hyst = int(prm[5])
    rising = int(prm[6])
    lo = level - hyst
    hi = level + hyst
    armed = 0
    i = 0
    while i < i1:
        v = int(buf[base + i * step])
        if rising != 0:
            if v < lo:
                armed = 1
            elif armed != 0:
                if v >= level:
                    armed = 0
                    if i >= i0:
                        return i
        else:
            if v > hi:
                armed = 1
            elif armed != 0:
                if v <= level:
                    armed = 0
                    if i >= i0:
                        return i
        i += 1
    return -1

@micropython.viper
def stats(buf: ptr16, prm: ptr32, res: ptr32):
    base = int(prm[0])
    step = int(prm[1])
    a = int(prm[2])
    b = int(prm[3])
    mn = 65535
    mx = 0
    sm = 0
    sq = 0
    i = a
    while i < b:
        v = int(buf[base + i * step])
        if v < mn:
            mn = v
        if v > mx:
            mx = v
        sm += v
        sq += (v * v) >> 4
        i += 1
    res[0] = mn
    res[1] = mx
    res[2] = sm
    res[3] = sq

@micropython.viper
def crossings(buf: ptr16, prm: ptr32, res: ptr32):
    base = int(prm[0])
    step = int(prm[1])
    a = int(prm[2])
    b = int(prm[3])
    mid = int(prm[4])
    lo = int(prm[5])
    armed = 0
    cnt = 0
    first = -1
    last = -1
    i = a
    while i < b:
        v = int(buf[base + i * step])
        if v < lo:
            armed = 1
        elif armed != 0:
            if v >= mid:
                armed = 0
                cnt += 1
                if first < 0:
                    first = i
                last = i
        i += 1
    res[4] = cnt
    res[5] = first
    res[6] = last

@micropython.viper
def highcount(buf: ptr16, prm: ptr32) -> int:
    base = int(prm[0])
    step = int(prm[1])
    a = int(prm[2])
    b = int(prm[3])
    mid = int(prm[4])
    cnt = 0
    i = a
    while i < b:
        if int(buf[base + i * step]) >= mid:
            cnt += 1
        i += 1
    return cnt

@micropython.viper
def columns(buf: ptr16, prm: ptr32, cols: ptr16):
    base = int(prm[0])
    step = int(prm[1])
    n = int(prm[7])
    s0 = int(prm[8])
    stp = int(prm[9])
    off = int(prm[10])
    px = 0
    while px < 160:
        a = (s0 + px * stp) >> 16
        b = (s0 + (px + 1) * stp) >> 16
        if a < 0:
            a = 0
        if a > n - 1:
            a = n - 1
        if b <= a:
            b = a + 1
        if b > n:
            b = n
        first = int(buf[base + a * step])
        lo = first
        hi = first
        last = first
        i = a
        while i < b:
            v = int(buf[base + i * step])
            if v < lo:
                lo = v
            if v > hi:
                hi = v
            last = v
            i += 1
        cols[off + px] = lo
        cols[off + 160 + px] = hi
        cols[off + 320 + px] = first
        cols[off + 480 + px] = last
        px += 1

def adc_stop():
    mem32[ADC_CS] = mem32[ADC_CS] & ~CS_START_MANY
    t0 = time.ticks_us()
    while not (mem32[ADC_CS] & CS_READY):
        if time.ticks_diff(time.ticks_us(), t0) > 5000:
            break
    k = 0
    while ((mem32[ADC_FCS] >> 16) & 0x1F) and k < 32:
        _ = mem32[ADC_FIFO]
        k += 1

# ============================ SETTINGS / STATE ============================
TIME_DIV = (0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0)   # ms/div
V_DIV    = (0.05, 0.1, 0.2, 0.5, 1.0, 2.0)
NPARAM   = 7

running    = True
status     = 'RUN'          # RUN / WAIT / HOLD / DONE
sel        = 0              # parameter terpilih
meas_page  = 0
td_index   = 3              # 1 ms/div
vd_index   = 4              # 1 V/div
trig_i     = 33             # level trigger = trig_i * 0.05 V  (1.65 V)
edge       = 0              # 0 = rising, 1 = falling
trig_mode  = 0              # 0 AUTO, 1 NORM, 2 SINGLE
ch_mode    = 0              # 0 = CH1, 1 = CH2, 2 = CH1+CH2
ctr_i      = 33             # titik tengah layar = ctr_i * 0.05 V ; -1 = AUTO (rata-rata)

cap_dirty = True            # capture perlu diulang (pengaturan berubah)
ui_dirty  = True            # teks perlu digambar ulang
need_draw = True            # ada yang perlu digambar
armed     = False
frame     = 0
dma_fail  = 0
wdt       = None

cap_meta = None             # (n, nch, dt, spp, i0, i1, chs)
cur_meta = None
cur_trig = 0
meas     = [None, None]

# ============================ CAPTURE =====================================
def tw_seconds():
    return H_DIVS * TIME_DIV[td_index] * 1e-3

def capture_config():
    nch = 2 if ch_mode == 2 else 1
    tw = tw_seconds()
    dt = tw / (2.0 * W)                       # target 2 sampel per pixel
    dt_min = nch * 96 / ADC_CLK               # 1 konversi = 96 clock ADC
    dt_max = nch * 65536 / ADC_CLK
    if dt < dt_min:
        dt = dt_min
    if dt > dt_max:
        dt = dt_max
    reg = int((dt * ADC_CLK / nch - 1) * 256)
    if reg < 95 * 256:
        reg = 95 * 256
    dt = nch * (1 + reg / 256) / ADC_CLK      # dt sebenarnya
    spp = tw / (W * dt)                       # sampel per pixel
    n = int(W * spp * 2.5)
    if n < 240:
        n = 240
    if n > NS_MAX:
        n = NS_MAX
    i0 = int(TRIG_X * spp) + 2
    i1 = n - int((W - TRIG_X) * spp) - 3
    if i1 <= i0 + 8:
        i0 = 2
        i1 = n - 2
    if ch_mode == 0:
        chs = ((0, 0, 1),); rr = 0; ains = 0
    elif ch_mode == 1:
        chs = ((1, 0, 1),); rr = 0; ains = 1
    else:
        chs = ((0, 0, 2), (1, 1, 2)); rr = 3; ains = 0
    return n, nch, dt, reg, rr, ains, spp, i0, i1, chs

def soft_capture(cfg):                        # cadangan kalau DMA tidak jalan
    global cap_meta
    n, nch, dt, reg, rr, ains, spp, i0, i1, chs = cfg
    if n > 640:
        n = 640
    b = cap_buf
    r0 = adc0.read_u16
    r1 = adc1.read_u16
    gap = int(dt * 1e6) - 12 * nch
    t0 = time.ticks_us()
    if ch_mode == 2:
        for i in range(n):
            b[2 * i] = r0() >> 4
            b[2 * i + 1] = r1() >> 4
            if gap > 0:
                time.sleep_us(gap)
    else:
        rd = r1 if ch_mode == 1 else r0
        for i in range(n):
            b[i] = rd() >> 4
            if gap > 0:
                time.sleep_us(gap)
    el = time.ticks_diff(time.ticks_us(), t0) * 1e-6
    dt_real = el / n
    spp = tw_seconds() / (W * dt_real)
    i0 = int(TRIG_X * spp) + 2
    i1 = n - int((W - TRIG_X) * spp) - 3
    if i1 <= i0 + 8:
        i0 = 2
        i1 = n - 2
    cap_meta = (n, nch, dt_real, spp, i0, i1, chs)

def abort_capture():
    global armed
    if USE_DMA and dma is not None:
        try:
            dma.active(0)
        except Exception:
            pass
        adc_stop()
    armed = False

def arm_capture():
    global armed, cap_meta, cap_dirty
    cap_dirty = False
    cfg = capture_config()
    n, nch, dt, reg, rr, ains, spp, i0, i1, chs = cfg
    if USE_DMA:
        abort_capture()
        mem32[ADC_DIV] = reg
        mem32[ADC_FCS] = FCS_EN | FCS_DREQ | (1 << 24) | FCS_CLR
        mem32[ADC_CS]  = CS_EN | (rr << 16) | (ains << 12)
        dma.config(read=ADC_FIFO, write=cap_buf, count=n * nch, ctrl=DMA_CTRL, trigger=True)
        mem32[ADC_CS]  = CS_EN | CS_START_MANY | (rr << 16) | (ains << 12)
        cap_meta = (n, nch, dt, spp, i0, i1, chs)
        armed = True
    else:
        soft_capture(cfg)
        armed = False

def wait_capture():
    """True = capture selesai normal. False = dibatalkan (setting berubah / HOLD / timeout)."""
    global armed, dma_fail, USE_DMA, need_draw
    if not USE_DMA:
        return True
    limit = int(cap_meta[0] * cap_meta[2] * 1000) + 600
    t0 = time.ticks_ms()
    while dma.active():
        handle_inputs()
        if cap_dirty or not running:
            abort_capture()
            return False
        if need_draw:
            draw_frame()                      # UI berubah -> gambar ulang sambil menunggu
        if time.ticks_diff(time.ticks_ms(), t0) > limit:
            abort_capture()
            dma_fail += 1
            if dma_fail >= 3:
                print("DMA tidak merespons -> pindah ke mode software")
                USE_DMA = False
            return False
    adc_stop()
    armed = False
    dma_fail = 0
    return True

# ============================ PENGUKURAN ==================================
def xtime(b, base, step, i, mid):
    y0 = b[base + (i - 1) * step]
    y1 = b[base + i * step]
    return (i - 1) + (mid - y0) / (y1 - y0)

def measure(b, chn, n, dt):
    ch, base, step = chn
    prm[0] = base; prm[1] = step; prm[2] = 0; prm[3] = n
    stats(b, prm, res)
    mn = res[0]; mx = res[1]; sm = res[2]; sq = res[3]
    cnt = n
    vpp = mx - mn
    frq = 0.0; per = 0.0; dty = 0.0
    if vpp > 60:
        mid = (mn + mx) >> 1
        prm[4] = mid
        prm[5] = mid - (vpp >> 3)
        crossings(b, prm, res)
        cc = res[4]; fi = res[5]; la = res[6]
        if cc >= 2:
            t1 = xtime(b, base, step, fi, mid)
            t2 = xtime(b, base, step, la, mid)
            per = (t2 - t1) * dt / (cc - 1)
            if per > 0:
                frq = 1.0 / per
            prm[2] = fi; prm[3] = la             # statistik dihitung di siklus utuh saja
            stats(b, prm, res)
            sm = res[2]; sq = res[3]; cnt = la - fi
            dty = highcount(b, prm) * 100.0 / cnt
    k = VREF / 4095.0
    return {
        "mx": mx * k, "mn": mn * k, "vpp": vpp * k,
        "avg": sm / cnt * k,
        "rms": math.sqrt(sq * 16.0 / cnt) * k,
        "frq": frq, "per": per, "dty": dty,
    }

def rebuild_columns():
    if cur_meta is None:
        return
    n, nch, dt, spp0, i0, i1, chs = cur_meta
    spp = tw_seconds() / (W * dt)
    stp = int(spp * 65536)
    if stp > (6 << 16):
        stp = 6 << 16
    if stp < 600:
        stp = 600
    s0 = (cur_trig << 16) - TRIG_X * stp
    if s0 < 0:
        s0 = 0
    for chn in chs:
        prm[0] = chn[1]; prm[1] = chn[2]; prm[7] = n
        prm[8] = s0; prm[9] = stp; prm[10] = chn[0] * 640
        columns(cur_buf, prm, cols)

def set_status(s):
    global status, ui_dirty, need_draw
    if s != status:
        status = s
        ui_dirty = True
        need_draw = True
        update_led()

def process_capture():
    global cap_buf, cur_buf, cur_meta, cur_trig, need_draw, running
    n, nch, dt, spp, i0, i1, chs = cap_meta
    ch, base, step = chs[0]
    prm[0] = base; prm[1] = step; prm[2] = i0; prm[3] = i1
    prm[4] = int(trig_i * 0.05 / VREF * 4095)
    prm[5] = TRIG_HYST
    prm[6] = 1 if edge == 0 else 0
    ti = find_trigger(cap_buf, prm)
    if ti < 0:
        if trig_mode == 0:                    # AUTO: tetap tampil walau tidak ada trigger
            ti = i0
        else:
            set_status('WAIT')
            return
    cur_buf, cap_buf = cap_buf, cur_buf       # tukar buffer
    cur_meta = cap_meta
    cur_trig = ti
    for chn in chs:
        meas[chn[0]] = measure(cur_buf, chn, n, dt)
    rebuild_columns()
    need_draw = True
    if trig_mode == 2:
        running = False
        set_status('DONE')
    else:
        set_status('RUN')

# ============================ DRAWING =====================================
def build_grid():
    global grid_img
    fb.fill(BLACK)
    for x in range(X0, X0 + GW, PX_PER_DIV_H):
        fb.vline(x, Y0, GH, DARK)
    fb.vline(X0 + GW - 1, Y0, GH, DARK)
    for k in range(-2, 3):
        fb.hline(X0, CENTER_Y + k * PX_PER_DIV_V, GW, DARK)
    fb.hline(X0, CENTER_Y, GW, G_CTR)
    fb.vline(X0 + GW // 2, Y0, GH, G_MID)
    for k in range(0, 26):                    # tick kecil 1/5 div
        fb.vline(int(k * 6.4), CENTER_Y - 1, 3, G_MID)
    for k in range(-10, 11):
        fb.hline(X0 + GW // 2 - 1, CENTER_Y + int(k * 3.6), 3, G_MID)
    grid_img = bytearray(buf[GS:GE])

def chan_center(ch):
    if ctr_i < 0:
        m = meas[ch]
        return m["avg"] if m else VREF / 2
    return ctr_i * 0.05

def draw_wave(ch, cv, ppv):
    off = ch * 640
    color = CH_COL[ch]
    B = int(ppv * VREF / 4095.0 * 65536)
    A = int((CENTER_Y + cv * ppv + 0.5) * 65536)
    ymin_c = Y0 + 1
    ymax_c = Y0 + GH - 3
    prev = -1000
    for px in range(W):
        yt = (A - cols[off + 160 + px] * B) >> 16      # dari nilai max (lebih ke atas)
        yb = (A - cols[off + px] * B) >> 16            # dari nilai min
        if prev != -1000:                              # sambung dengan kolom sebelumnya
            if prev < yt:
                yt = prev
            if prev > yb:
                yb = prev
        prev = (A - cols[off + 480 + px] * B) >> 16
        if yb < ymin_c:
            yt = ymin_c; yb = ymin_c
        elif yt > ymax_c:
            yt = ymax_c; yb = ymax_c
        else:
            if yt < ymin_c:
                yt = ymin_c
            if yb > ymax_c:
                yb = ymax_c
        h = yb - yt + 1
        if h < 2:
            h = 2
        fb.vline(px, yt, h, color)
    gy = int(CENTER_Y + cv * ppv)                      # penanda 0 V
    if Y0 + 2 <= gy <= Y0 + GH - 3:
        fb.fill_rect(0, gy - 1, 4, 3, color)

def draw_trig_marker(cv, ppv):
    ty = int(CENTER_Y - (trig_i * 0.05 - cv) * ppv)
    if Y0 + 4 <= ty <= Y0 + GH - 5:
        for k in range(4):
            fb.vline(W - 4 + k, ty - k, 2 * k + 1, YELLOW)
    fb.hline(TRIG_X - 2, Y0, 5, YELLOW)                # penanda waktu trigger
    fb.hline(TRIG_X - 1, Y0 + 1, 3, YELLOW)
    fb.pixel(TRIG_X, Y0 + 2, YELLOW)

def fmt_freq(f):
    if f <= 0:
        return "---"
    if f >= 1000:
        return "{:.2f}kHz".format(f / 1000)
    return "{:.1f}Hz".format(f)

def fmt_per(p):
    if p <= 0:
        return "---"
    if p >= 1e-3:
        return "{:.2f}ms".format(p * 1000)
    return "{:.1f}us".format(p * 1e6)

def meas_rows(m, page, k):
    if page == 0:
        return ("Vpp:{:.2f}V".format(m["vpp"]),
                "Avg:{:.2f}V".format(m["avg"]),
                "Frq:" + fmt_freq(m["frq"]))
    if page == 1:
        return ("Max:{:.2f}V".format(m["mx"]),
                "Min:{:.2f}V".format(m["mn"]),
                "Rms:{:.2f}V".format(m["rms"]))
    third = ""
    if k == 1 and meas[0] and meas[1] and meas[0]["vpp"] > 0.02:
        third = "Gn:{:.0f}%".format(meas[1]["vpp"] / meas[0]["vpp"] * 100)
    dty = "Dty:{:.0f}%".format(m["dty"]) if m["frq"] > 0 else "Dty:---"
    return ("Per:" + fmt_per(m["per"]), dty, third)

def draw_top():
    fb.fill_rect(0, 0, W, 23, BLACK)
    if cur_meta is None:
        draw_text(2, 8, "Menunggu data...", GRAY)
        return
    chs = cur_meta[6]
    if len(chs) == 2:
        for k in range(2):
            m = meas[chs[k][0]]
            rows = meas_rows(m, meas_page, k)
            x = 2 if k == 0 else 84
            for r in range(3):
                draw_text(x, 2 + 8 * r, rows[r], CH_COL[chs[k][0]])
    else:
        ch = chs[0][0]
        m = meas[ch]
        for k in range(2):
            rows = meas_rows(m, (meas_page + k) % 3, -1)
            x = 2 if k == 0 else 84
            for r in range(3):
                draw_text(x, 2 + 8 * r, rows[r], CH_COL[ch])

def draw_bottom():
    fb.fill_rect(0, 99, W, 29, BLACK)
    fb.hline(0, 99, W, DARK)
    cen = "AUTO" if ctr_i < 0 else "{:.2f}V".format(ctr_i * 0.05)
    fields = (
        (0, 2,   103, "T:{:g}ms".format(TIME_DIV[td_index])),
        (1, 56,  103, "V:{:g}V".format(V_DIV[vd_index])),
        (2, 2,   111, "Lv:{:.2f}V".format(trig_i * 0.05)),
        (3, 56,  111, "Edge:" + ("Rise" if edge == 0 else "Fall")),
        (4, 112, 111, "Mode:" + ("AUTO", "NORM", "SNGL")[trig_mode]),
        (5, 2,   119, "CH:" + ("1", "2", "1+2")[ch_mode]),
        (6, 56,  119, "Ctr:" + cen),
    )
    for p, x, y, s in fields:
        col = YELLOW if sel == p else GRAY
        draw_text(x, y, s, col)
        if sel == p:
            fb.hline(x, y + 7, text_w(s), YELLOW)
    if cur_meta is not None:
        draw_text(112, 119, "Fs:{:.3g}k".format(1.0 / cur_meta[2] / 1000), GRAY)
    if status == 'RUN':
        bg = GREEN; fg = BLACK
    elif status == 'WAIT':
        bg = YELLOW; fg = BLACK
    else:
        bg = RED; fg = WHITE
    fb.fill_rect(112, 101, 46, 9, bg)
    draw_text(112 + (46 - text_w(status)) // 2, 102, status, fg)

def draw_frame():
    global need_draw, ui_dirty, frame
    frame += 1
    mv[GS:GE] = grid_img
    if cur_meta is not None:
        ppv = PX_PER_DIV_V / V_DIV[vd_index]
        chs = cur_meta[6]
        for chn in chs:
            draw_wave(chn[0], chan_center(chn[0]), ppv)
        src = chs[0][0]
        draw_trig_marker(chan_center(src), ppv)
    if ui_dirty or frame % 5 == 0:
        draw_top()
        draw_bottom()
        ui_dirty = False
    show()
    need_draw = False

# ============================ INPUT =======================================
ENC_TABLE = (0,1,-1,0, -1,0,0,1, 1,0,0,-1, 0,-1,1,0)
enc_state = (clk.value() << 1) | dt_pin.value()
enc_count = 0
sw_enc = [1, 0, 0]
sw_btn = [1, 0, 0]

def enc_isr(pin):
    global enc_state, enc_count
    s = (clk.value() << 1) | dt_pin.value()
    enc_count += ENC_TABLE[(enc_state << 2) | s]
    enc_state = s

both = Pin.IRQ_FALLING | Pin.IRQ_RISING
clk.irq(trigger=both, handler=enc_isr)
dt_pin.irq(trigger=both, handler=enc_isr)

def poll_switch(pin, st):
    """0 = tidak ada, 1 = tekan singkat, 2 = tahan lama"""
    now = time.ticks_ms()
    v = pin.value()
    r = 0
    if v == 0:
        if st[0] == 1:
            st[1] = now
            st[2] = 0
        elif st[2] == 0 and time.ticks_diff(now, st[1]) >= LONG_MS:
            st[2] = 1
            r = 2
    elif st[0] == 0 and st[2] == 0 and time.ticks_diff(now, st[1]) > 30:
        r = 1
    st[0] = v
    return r

def clamp(v, lo, hi):
    return lo if v < lo else (hi if v > hi else v)

def adjust(p, steps):
    global td_index, vd_index, trig_i, edge, trig_mode, ch_mode, ctr_i
    global cap_dirty, ui_dirty, need_draw
    if p == 0:
        td_index = clamp(td_index + steps, 0, len(TIME_DIV) - 1)
        cap_dirty = True
        if not running:
            rebuild_columns()                 # di HOLD: zoom horizontal dari data yang tersimpan
    elif p == 1:
        vd_index = clamp(vd_index + steps, 0, len(V_DIV) - 1)
    elif p == 2:
        trig_i = clamp(trig_i + steps, 0, 66)
        cap_dirty = True
    elif p == 3:
        edge = (edge + steps) % 2
        cap_dirty = True
    elif p == 4:
        trig_mode = (trig_mode + steps) % 3
        cap_dirty = True
    elif p == 5:
        ch_mode = (ch_mode + steps) % 3
        cap_dirty = True
    elif p == 6:
        ctr_i = clamp(ctr_i + steps, -1, 66)
    ui_dirty = True
    need_draw = True

def autoset():
    global td_index, vd_index, trig_i, edge, trig_mode, ctr_i
    global cap_dirty, ui_dirty, need_draw
    src = 1 if ch_mode == 1 else 0
    m = meas[src]
    if not m:
        return
    if m["frq"] > 0:
        target = 500.0 / m["frq"]             # ms/div supaya tampil sekitar 2,5 siklus
        best = 0
        bd = 1e9
        for i in range(len(TIME_DIV)):
            d = abs(math.log(TIME_DIV[i] / target))
            if d < bd:
                bd = d
                best = i
        td_index = best
    vd_index = len(V_DIV) - 1
    for i in range(len(V_DIV)):
        if m["vpp"] / V_DIV[i] <= 3.6:
            vd_index = i
            break
    trig_i = clamp(int((m["mx"] + m["mn"]) / 2 / 0.05 + 0.5), 0, 66)
    edge = 0
    trig_mode = 0
    ctr_i = -1
    cap_dirty = True
    ui_dirty = True
    need_draw = True

def handle_inputs():
    global enc_count, sel, meas_page, running, cap_dirty, ui_dirty, need_draw
    if wdt:
        wdt.feed()
    irq = disable_irq()
    n = enc_count
    steps = int(n / ENC_DIV)
    enc_count -= steps * ENC_DIV
    enable_irq(irq)
    if steps:
        if ENC_REV:
            steps = -steps
        adjust(sel, steps)
    r = poll_switch(enc_sw, sw_enc)
    if r == 1:
        sel = (sel + 1) % NPARAM
        ui_dirty = True
        need_draw = True
    elif r == 2:
        autoset()
    r = poll_switch(button, sw_btn)
    if r == 1:
        if running:
            running = False
            set_status('HOLD')
        else:
            running = True
            cap_dirty = True
            set_status('RUN')
    elif r == 2:
        meas_page = (meas_page + 1) % 3
        ui_dirty = True
        need_draw = True

# ============================ MAIN ========================================
def loop_once():
    handle_inputs()
    if running:
        arm_capture()
        if need_draw:
            draw_frame()                      # gambar frame lama sambil DMA mengambil data baru
        if wait_capture():
            process_capture()
    else:
        if armed:
            abort_capture()
        if need_draw:
            draw_frame()
        else:
            time.sleep_ms(10)

def setup():
    global wdt
    init_tft()
    build_grid()
    show()
    update_led()
    print("Pico scope | chip:", "RP2350" if IS_2350 else "RP2040",
          "| CPU:", machine.freq() // 1000000, "MHz | DMA:", USE_DMA)
    if USE_WDT:
        wdt = WDT(timeout=8000)

def main():
    setup()
    while True:
        loop_once()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        sys.print_exception(e)
        if AUTO_RESET:
            time.sleep_ms(500)
            machine.reset()
