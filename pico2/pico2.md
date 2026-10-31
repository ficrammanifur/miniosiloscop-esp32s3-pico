# 🔬 DIY Digital Oscilloscope & Signal Generator

<p align="center">

**A Two-Board Electronic Instrumentation Project**

Signal Generator berbasis ESP32-S3 dan Digital Oscilloscope berbasis Raspberry Pi Pico 2 (RP2350).

![ESP32](https://img.shields.io/badge/ESP32--S3-Signal%20Generator-red)
![RP2350](https://img.shields.io/badge/Raspberry%20Pi%20Pico%202-Oscilloscope-green)
![MicroPython](https://img.shields.io/badge/Oscilloscope-MicroPython-yellow)
![Arduino](https://img.shields.io/badge/Generator-Arduino%20IDE-blue)
![Display](https://img.shields.io/badge/Display-TFT%20SPI-orange)
![Project](https://img.shields.io/badge/Project-DIY%20Electronics-purple)

</p>

---

## 📌 1. Project Overview

Proyek ini merupakan pengembangan instrumen elektronika digital DIY yang menggabungkan dua mikrokontroler, yaitu **ESP32-S3** sebagai Signal Generator dan **Raspberry Pi Pico 2 (RP2350)** sebagai Digital Oscilloscope.

Kedua perangkat bekerja secara terpisah tetapi saling terhubung melalui jalur sinyal analog.

ESP32-S3 bertugas menghasilkan berbagai bentuk sinyal listrik, seperti square wave, PWM, pulse, dan sine wave berbasis PWM.

Sementara itu, Raspberry Pi Pico 2 bertugas mengambil sampel sinyal menggunakan ADC internal dengan mekanisme DMA, kemudian memproses dan menampilkan waveform pada TFT SPI ST7735.

Sistem ini juga dilengkapi dengan fitur pengukuran otomatis untuk mengetahui karakteristik sinyal yang dihasilkan.

### Tujuan Proyek

- Mengembangkan signal generator digital berbasis ESP32-S3.
- Mengembangkan digital oscilloscope berbasis RP2350.
- Mengimplementasikan DMA untuk akuisisi data ADC.
- Memvisualisasikan waveform secara real-time.
- Mengukur frekuensi, tegangan, periode, dan duty cycle.
- Menguji karakteristik rangkaian elektronik menggunakan sinyal referensi.
- Mengembangkan platform eksperimen instrumentasi elektronika DIY.

---

## 🏗️ 2. System Architecture

Sistem terdiri dari dua perangkat utama.

### A. Signal Generator — ESP32-S3

Bertugas menghasilkan sinyal pengujian dengan beberapa konfigurasi frekuensi dan amplitudo.

Fitur:
- Hardware PWM.
- Square wave generator.
- PWM generator.
- Pulse generator.
- Sine wave generator berbasis PWM.
- Pengaturan amplitudo menggunakan tombol BOOT.
- Pengaturan frekuensi menggunakan long press.

### B. Digital Oscilloscope — Raspberry Pi Pico 2

Bertugas membaca dan menganalisis sinyal dari ESP32-S3.

Fitur:
- ADC sampling menggunakan DMA.
- Dual channel input.
- Hardware trigger.
- Rising dan falling edge.
- AUTO, NORM, dan SINGLE mode.
- Pre-trigger.
- Min/Max envelope rendering.
- Automatic measurement.
- AUTOSET.
- RUN / HOLD.
- TFT waveform visualization.

### Alur Kerja Sistem

```text
       ESP32-S3
    SIGNAL GENERATOR
           |
           |
    +------+------+
    |      |      |
  GPIO4  GPIO5  GPIO6
  GPIO7  GPIO15
    |      |      |
    +------+------+
           |
           v
    SIGNAL INPUT
           |
           v
   RASPBERRY PI PICO 2
        RP2350
           |
     +-----+------+
     |            |
    ADC          DMA
     |            |
     +-----+------+
           |
      DATA PROCESSING
           |
    +------+------+ 
    |             |
 WAVEFORM      MEASUREMENT
 RENDERING      ANALYSIS
    |             |
    +------+------+
           |
           v
       TFT DISPLAY
       ST7735
```

---

## ⚙️ 3. Hardware Specifications

### Signal Generator — ESP32-S3

| Parameter | Specification |
|---|---|
| Microcontroller | ESP32-S3 |
| Programming | Arduino C++ |
| Framework | Arduino-ESP32 Core 3.x |
| Signal Output | 5 GPIO outputs |
| PWM | LEDC Hardware PWM |
| Button | BOOT GPIO0 |
| Serial Monitor | 115200 baud |

### Digital Oscilloscope — Raspberry Pi Pico 2

| Parameter | Specification |
|---|---|
| Microcontroller | Raspberry Pi Pico 2 |
| Chip | RP2350 |
| Programming | MicroPython |
| CPU Configuration | 200 MHz |
| ADC | Internal 12-bit |
| Reference Voltage | 3.3 V assumed |
| Sampling | DMA |
| Channel | 2 channels |
| Display | TFT SPI ST7735 |
| Resolution | 160 × 128 |
| Maximum Sample Buffer | 1024 samples per channel |
| Rendering | Min/Max Envelope |
| Input Control | Rotary Encoder + Push Button |

---

## 📡 4. Signal Generator Specifications

ESP32-S3 menyediakan lima output sinyal.

| GPIO | Signal Type | Frequency | Duty Cycle |
|---|---|---:|---:|
| GPIO 4 | Square Wave | 1 kHz | 50% |
| GPIO 5 | PWM | 500 Hz | 25% |
| GPIO 6 | Pulse | 20 Hz | 50% |
| GPIO 7 | Square Wave | 100 Hz | 50% |
| GPIO 15 | Sine Wave | 25–200 Hz | Analog-like |

### GPIO 15 — Sine Wave Generator

Sinyal sinus dibangkitkan menggunakan tabel sinus 64 titik yang mengatur duty cycle PWM.

Konfigurasi:

| Parameter | Nilai |
|---|---|
| PWM Carrier | 60 kHz |
| Sine Table | 64 samples |
| PWM Resolution | 10-bit |
| Frequency | 25, 50, 100, 200 Hz |
| Amplitude | 4 selectable levels |

### Amplitude Selection

| Mode | Target Vpp |
|---|---:|
| Level 1 | 3.30 V |
| Level 2 | 1.65 V |
| Level 3 | 0.825 V |
| Level 4 | 0.330 V |

Nilai tersebut merupakan target teoritis berdasarkan asumsi swing output 3,3 V. Hasil aktual perlu diukur.

### ESP32-S3 Button Control

| Aksi BOOT | Fungsi |
|---|---|
| Tekan singkat | Mengubah amplitudo |
| Tahan > 0.7 detik | Mengubah frekuensi |

Frekuensi berubah secara berurutan:

`25 Hz → 50 Hz → 100 Hz → 200 Hz`

Amplitudo berubah secara berurutan:

`3.30 Vpp → 1.65 Vpp → 0.825 Vpp → 0.330 Vpp`

---

## 📊 5. Oscilloscope Specifications

Raspberry Pi Pico 2 menggunakan ADC internal yang dikombinasikan dengan DMA untuk pengambilan data.

### Sampling Configuration

| Mode | Target Sampling |
|---|---:|
| Single Channel | Up to 500 ksps |
| Dual Channel | Up to 250 ksps/channel |

Angka tersebut merupakan target konfigurasi, bukan hasil benchmark terverifikasi.

### Trigger System

Tersedia tiga mode trigger:

| Mode | Fungsi |
|---|---|
| AUTO | Tetap menampilkan waveform tanpa trigger |
| NORM | Menunggu trigger valid |
| SINGLE | Mengambil satu capture setelah trigger |

Trigger edge:

- Rising Edge
- Falling Edge

### Time Division

| Setting |
|---|
| 0.1 ms/div |
| 0.2 ms/div |
| 0.5 ms/div |
| 1 ms/div |
| 2 ms/div |
| 5 ms/div |
| 10 ms/div |
| 20 ms/div |
| 50 ms/div |
| 100 ms/div |

### Voltage Division

| Setting |
|---|
| 0.05 V/div |
| 0.1 V/div |
| 0.2 V/div |
| 0.5 V/div |
| 1 V/div |
| 2 V/div |

---

## 📈 6. Measurement Features

Oscilloscope dapat menghitung beberapa parameter sinyal secara otomatis.

| Measurement | Description |
|---|---|
| Vpp | Peak-to-peak voltage |
| Avg | Average voltage |
| RMS | Root Mean Square |
| Max | Maximum voltage |
| Min | Minimum voltage |
| Frequency | Signal frequency |
| Period | Signal period |
| Duty Cycle | High-state percentage |
| Gain | CH2 Vpp / CH1 Vpp |

### Measurement Pages

**Page 1 — Basic Measurement**
- Vpp
- Average
- Frequency

**Page 2 — Voltage Measurement**
- Maximum
- Minimum
- RMS

**Page 3 — Time Measurement**
- Period
- Duty Cycle
- Gain

Halaman pengukuran dapat diganti dengan menahan push button pada Pico 2.

---

## 🔌 7. Hardware Wiring

### A. ESP32-S3 Signal Output

| ESP32-S3 GPIO | Function |
|---|---|
| GPIO 4 | 1 kHz Square |
| GPIO 5 | 500 Hz PWM |
| GPIO 6 | 20 Hz Pulse |
| GPIO 7 | 100 Hz Square |
| GPIO 15 | Sine Wave |

### B. ESP32-S3 to Pico 2

Gunakan satu output untuk satu pengujian.

| ESP32-S3 | Raspberry Pi Pico 2 |
|---|---|
| GPIO 4 | GP26 (CH1) |
| GPIO 5 | GP26 (CH1) |
| GPIO 6 | GP26 (CH1) |
| GPIO 7 | GP26 (CH1) |
| GPIO 15 | GP26 (CH1) |
| GND | GND |

Untuk pengujian dual channel:

| Signal | Pico 2 |
|---|---|
| ESP32-S3 GPIO 4 | GP26 (CH1) |
| ESP32-S3 GPIO 7 | GP27 (CH2) |
| GND | GND |

**Catatan:** Jangan menghubungkan seluruh output GPIO ke satu input secara bersamaan. Gunakan jumper untuk memilih sinyal yang sedang diuji.

### C. Pico 2 TFT Display

| TFT | Pico 2 |
|---|---|
| SCK | GP18 |
| MOSI | GP19 |
| DC | GP16 |
| CS | GP17 |
| RST | GP20 |

### D. Pico 2 Rotary Encoder

| Encoder | Pico 2 |
|---|---|
| CLK | GP10 |
| DT | GP11 |
| SW | GP12 |
| GND | GND |

### E. Pico 2 Push Button

| Button | Pico 2 |
|---|---|
| Signal | GP13 |
| GND | GND |

### F. Pico 2 RGB LED

| RGB | Pico 2 |
|---|---|
| Red | GP2 |
| Green | GP3 |
| Blue | GP4 |

---

## 🛠️ 8. Software Requirements

### ESP32-S3

- Arduino IDE.
- Arduino-ESP32 Core 3.x.
- USB connection.
- Serial Monitor.

Library tambahan tidak diperlukan untuk program signal generator ini.

### Raspberry Pi Pico 2

- Thonny IDE.
- MicroPython firmware khusus Pico 2.
- USB connection.

Download firmware:

https://micropython.org/download/RPI_PICO2/

---

## 🚀 9. Installation Guide

### Step 1 — Upload ESP32-S3 Signal Generator

1. Buka Arduino IDE.
2. Pilih board ESP32-S3 yang sesuai.
3. Pastikan Arduino-ESP32 Core versi 3.x.
4. Masukkan program Signal Generator.
5. Hubungkan ESP32-S3 melalui USB.
6. Upload program.
7. Buka Serial Monitor dengan baud rate 115200.

Output awal Serial Monitor:

```text
ESP32-S3 SIGNAL GENERATOR
GPIO 4  : Square 1 kHz
GPIO 5  : PWM 500 Hz / 25%
GPIO 6  : Pulse 20 Hz
GPIO 7  : Square 100 Hz
GPIO 15 : Sinus (PWM 60 kHz)
```

### Step 2 — Install Pico 2 MicroPython

1. Hubungkan Pico 2 sambil menekan BOOTSEL.
2. Salin firmware UF2 Pico 2.
3. Buka Thonny IDE.
4. Pilih interpreter MicroPython Raspberry Pi Pico.
5. Pastikan perangkat terdeteksi.

### Step 3 — Upload Oscilloscope Program

Simpan kode oscilloscope dengan nama:

`main.py`

Kemudian:

1. Upload ke Raspberry Pi Pico 2.
2. Pastikan wiring TFT sesuai.
3. Pastikan encoder dan tombol terpasang.
4. Restart perangkat.

### Step 4 — Connect Both Devices

1. Hubungkan GND ESP32-S3 dan GND Pico 2.
2. Pilih salah satu GPIO output ESP32-S3.
3. Hubungkan output tersebut ke GP26.
4. Nyalakan kedua perangkat.
5. Pilih CH1 pada oscilloscope.
6. Amati waveform pada TFT.

---

## 🎮 10. Oscilloscope Controls

### Rotary Encoder

| Action | Function |
|---|---|
| Rotate | Change selected parameter |
| Short Press | Select next parameter |
| Long Press 0.7 s | AUTOSET |

Urutan parameter:

`T/DIV → V/DIV → LEVEL → EDGE → MODE → CH → CTR`

### Push Button

| Action | Function |
|---|---|
| Short Press | RUN / HOLD |
| Long Press 0.7 s | Change measurement page |

### RGB LED Status

| Status | LED Indicator |
|---|---|
| RUN | Green |
| WAIT | Orange |
| HOLD / DONE | Red |

---

## 🧪 11. Testing and Experiments

Berikut beberapa eksperimen yang dapat dilakukan menggunakan kedua perangkat.

### Experiment 1 — Square Wave Measurement

Source: ESP32-S3 GPIO 4.

Expected values:

| Parameter | Expected |
|---|---:|
| Frequency | 1 kHz |
| Period | 1 ms |
| Vpp | Approximately 3.3 V |
| Duty Cycle | 50% |

### Experiment 2 — PWM Measurement

Source: ESP32-S3 GPIO 5.

| Parameter | Expected |
|---|---:|
| Frequency | 500 Hz |
| Period | 2 ms |
| Vpp | Approximately 3.3 V |
| Duty Cycle | 25% |

### Experiment 3 — Pulse Measurement

Source: ESP32-S3 GPIO 6.

| Parameter | Expected |
|---|---:|
| Frequency | 20 Hz |
| Period | 50 ms |
| Duty Cycle | 50% |

### Experiment 4 — Square Wave Measurement

Source: ESP32-S3 GPIO 7.

| Parameter | Expected |
|---|---:|
| Frequency | 100 Hz |
| Period | 10 ms |
| Vpp | Approximately 3.3 V |
| Duty Cycle | 50% |

### Experiment 5 — Sine Wave Measurement

Source: ESP32-S3 GPIO 15.

| Parameter | Expected |
|---|---|
| Frequency | 25–200 Hz |
| Amplitude | 4 selectable levels |
| Waveform | PWM-based sine approximation |
| Offset | Approximately 1.65 V |

### Experiment 6 — RC Low Pass Filter

Gunakan output GPIO 15 sebagai input rangkaian RC low-pass filter.

Bandingkan:
- Waveform sebelum filter.
- Waveform sesudah filter.
- Perubahan Vpp.
- Ripple PWM.
- Perubahan bentuk sinus.

### Experiment 7 — Dual Channel Measurement

Gunakan dua output ESP32-S3 yang berbeda.

Contoh:
- CH1: GPIO 4 (1 kHz).
- CH2: GPIO 7 (100 Hz).

Amati kedua waveform dan bandingkan hasil pengukuran masing-masing channel.

---

## ⚠️ 12. Important Technical Notes

1. Semua input ADC Pico 2 harus berada dalam rentang tegangan yang diizinkan, yaitu 0–3,3 V nominal untuk konfigurasi proyek ini.
2. ESP32-S3 dan Pico 2 harus menggunakan ground bersama.
3. GPIO 15 menghasilkan sinus melalui PWM, bukan DAC sinus murni.
4. Low-pass filter dapat digunakan untuk mengurangi komponen switching PWM.
5. GPIO 6 dan GPIO 7 menggunakan software timing sehingga dapat memiliki jitter.
6. Pengukuran RMS mencakup DC offset.
7. Sampling rate aktual perlu diverifikasi melalui pengujian.
8. Pengukuran dual channel dilakukan secara bergantian, bukan simultan.
9. Proyek ini bukan instrumen terkalibrasi untuk pengukuran keselamatan atau tegangan tinggi.
10. Jangan menghubungkan input ADC langsung ke sumber tegangan negatif atau tegangan yang melebihi batasnya.

---

## 🔮 13. Future Development

Rencana pengembangan selanjutnya:

- [ ] Automatic ADC Voltage Calibration.
- [ ] Input Attenuator.
- [ ] Input Protection Circuit.
- [ ] AC/DC Coupling.
- [ ] Probe x1 / x10.
- [ ] Adjustable RC Low Pass Filter.
- [ ] FFT Spectrum Analyzer.
- [ ] Phase Difference Measurement.
- [ ] Waveform Data Logging.
- [ ] USB Serial Data Export.
- [ ] PC Oscilloscope Interface.
- [ ] Automatic Measurement Error Analysis.

---

## 📁 14. Project Structure

```text
DIY-Digital-Oscilloscope/
│
├── README.md
│
├── Signal-Generator/
│   └── signal_generator.ino
│
├── Digital-Oscilloscope/
│   └── main.py
│
├── Hardware/
│   ├── wiring-diagram.png
│   └── circuit-diagram.png
│
└── Documentation/
    └── experiment-results.md
```

---

## 👨‍💻 15. Project Information

**Project Name:** DIY Digital Oscilloscope & Signal Generator

**Main Hardware:**
- ESP32-S3
- Raspberry Pi Pico 2 (RP2350)
- TFT SPI ST7735
- Rotary Encoder
- Push Button
- RGB LED

**Development Environment:**
- Arduino IDE
- Thonny IDE
- Arduino C++
- MicroPython

**Project Category:**
- Embedded Systems
- Digital Electronics
- Signal Processing
- Electronic Instrumentation
- Hardware Development

---

<p align="center">

**"From Signal Generation to Real-Time Signal Analysis."**

Built with curiosity. Developed through experimentation.

🔬 DIY Electronics | Embedded Systems | Instrumentation

</p>
