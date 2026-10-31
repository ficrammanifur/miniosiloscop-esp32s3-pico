# 🔬 DMA Mini Oscilloscope — Raspberry Pi Pico 2

<p align="center">

**High-Speed Dual Channel Digital Oscilloscope Based on RP2350**

Dibangun menggunakan Raspberry Pi Pico 2, MicroPython, DMA, ADC, dan TFT SPI Display.

![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico%202-green)
![MCU](https://img.shields.io/badge/MCU-RP2350-blue)
![Language](https://img.shields.io/badge/Language-MicroPython-yellow)
![Display](https://img.shields.io/badge/Display-TFT%20SPI-orange)
![Sampling](https://img.shields.io/badge/Sampling-Up%20to%20500%20ksps-red)
![Status](https://img.shields.io/badge/Project-Development-purple)

</p>

---

## 📌 1. Tentang Proyek

DMA Mini Oscilloscope merupakan proyek pengembangan osiloskop digital berbasis **Raspberry Pi Pico 2 (RP2350)** yang dirancang untuk membaca, memvisualisasikan, dan menganalisis sinyal listrik secara real-time.

Proyek ini menggunakan ADC internal Pico 2 dengan mekanisme **Direct Memory Access (DMA)** untuk mengambil sampel sinyal tanpa harus membaca ADC satu per satu melalui proses Python biasa.

Data hasil sampling kemudian diproses dan ditampilkan pada layar TFT SPI beresolusi 160 × 128 piksel.

Sistem ini dilengkapi dengan dua channel input, trigger yang dapat dikonfigurasi, pengukuran karakteristik sinyal, serta kontrol interaktif menggunakan rotary encoder dan push button.

### 🎯 Tujuan Pengembangan

- Membuat osiloskop digital DIY dengan biaya relatif terjangkau.
- Memanfaatkan kemampuan ADC dan DMA pada RP2350.
- Menampilkan bentuk gelombang secara real-time.
- Mengukur parameter dasar sinyal listrik.
- Mengembangkan sistem pengukuran elektronik berbasis MicroPython.

---

## ⚡ 2. Fitur Utama

| Fitur | Keterangan |
|---|---|
| DMA Sampling | Pengambilan sampel menggunakan DMA |
| Dual Channel | CH1 dan CH2 |
| Trigger | Rising Edge dan Falling Edge |
| Trigger Mode | AUTO, NORM, SINGLE |
| Pre-trigger | Menampilkan data sebelum titik trigger |
| Envelope Rendering | Menampilkan nilai minimum dan maksimum per kolom |
| Autoset | Pengaturan skala otomatis |
| RUN / HOLD | Menjalankan atau membekukan tampilan |
| Real-time Measurement | Pengukuran parameter sinyal |
| Rotary Encoder | Pengaturan parameter secara interaktif |
| RGB LED | Indikator status perangkat |

### 📊 Parameter Pengukuran

Sistem dapat menampilkan beberapa parameter berikut:

| Parameter | Fungsi |
|---|---|
| Vpp | Tegangan peak-to-peak |
| Avg | Tegangan rata-rata |
| RMS | Root Mean Square |
| Max | Tegangan maksimum |
| Min | Tegangan minimum |
| Frequency | Frekuensi sinyal |
| Period | Periode sinyal |
| Duty Cycle | Persentase waktu sinyal HIGH |
| Gain | Perbandingan Vpp CH2 terhadap CH1 |

---

## 🧠 3. Spesifikasi Sistem

| Komponen | Spesifikasi |
|---|---|
| Microcontroller | Raspberry Pi Pico 2 |
| Processor | RP2350 |
| Programming Language | MicroPython |
| ADC | Internal 12-bit |
| ADC Resolution | 0–4095 |
| Reference Voltage | 3.3 V (asumsi kalibrasi kode) |
| Sampling Method | DMA |
| Maximum Sampling | Target hingga 500 ksps |
| Channel | 2 Channel |
| Display | TFT SPI ST7735 |
| Resolution | 160 × 128 |
| Communication | SPI |
| Waveform Rendering | Min/Max Envelope |
| Buffer | Double Buffer |
| CPU Frequency | 200 MHz (konfigurasi awal) |

**Catatan:** Angka 500 ksps merupakan target konfigurasi sampling satu channel, bukan hasil pengujian akurasi yang sudah terverifikasi. Pada mode dua channel, targetnya sekitar 250 ksps per channel. Kecepatan aktual bergantung pada konfigurasi ADC, DMA, firmware, dan kestabilan perangkat.

---

## 🔌 4. Wiring dan Konfigurasi Pin

### 4.1 TFT SPI Display

| TFT Pin | Pico 2 GPIO |
|---|---|
| SCK | GP18 |
| MOSI | GP19 |
| DC | GP16 |
| CS | GP17 |
| RST | GP20 |
| VCC | Sesuai spesifikasi modul |
| GND | GND |

### 4.2 ADC Input

| Channel | GPIO | ADC |
|---|---|---|
| CH1 | GP26 | ADC0 |
| CH2 | GP27 | ADC1 |

Input ADC menggunakan rentang tegangan yang aman untuk Raspberry Pi Pico 2.

**Penting:** Jangan menghubungkan tegangan negatif atau tegangan di atas batas ADC secara langsung. Gunakan rangkaian pembagi tegangan, proteksi input, dan pengondisi sinyal apabila mengukur sinyal dari rangkaian eksternal.

### 4.3 Rotary Encoder

| Encoder | Pico 2 GPIO |
|---|---|
| CLK | GP10 |
| DT | GP11 |
| SW | GP12 |
| GND | GND |

### 4.4 Push Button

| Button | Pico 2 GPIO |
|---|---|
| Signal | GP13 |
| GND | GND |

### 4.5 RGB LED

| RGB LED | Pico 2 GPIO |
|---|---|
| Red | GP2 |
| Green | GP3 |
| Blue | GP4 |
| Common | Sesuai jenis RGB LED |

Gunakan resistor pembatas arus pada masing-masing jalur LED.

---

## 🛠️ 5. Persiapan dan Instalasi

### Perangkat yang Dibutuhkan

- Raspberry Pi Pico 2 (RP2350).
- TFT SPI ST7735 160 × 128.
- Rotary Encoder.
- Push Button.
- RGB LED.
- Kabel jumper.
- Breadboard atau PCB.
- Kabel USB untuk pemrograman.

### Software

1. Thonny IDE.
2. Firmware MicroPython khusus Raspberry Pi Pico 2.
3. USB connection ke komputer.

### Langkah Instalasi

**1. Install MicroPython**

Download firmware MicroPython khusus Raspberry Pi Pico 2 dari:

https://micropython.org/download/RPI_PICO2/

Pastikan firmware yang digunakan kompatibel dengan RP2350.

**2. Hubungkan Pico 2**

- Tekan tombol BOOTSEL.
- Hubungkan Pico 2 ke komputer melalui USB.
- Lepaskan tombol BOOTSEL.
- Salin firmware UF2 ke drive yang muncul.

**3. Buka Thonny IDE**

Pilih interpreter:

`MicroPython (Raspberry Pi Pico)`

Pastikan perangkat terdeteksi dengan benar.

**4. Upload Program**

1. Buka kode Python osiloskop.
2. Pastikan seluruh wiring sudah sesuai.
3. Simpan file dengan nama:

`main.py`

4. Pilih Save to Raspberry Pi Pico 2.
5. Restart perangkat.

Jika konfigurasi benar, osiloskop akan melakukan inisialisasi TFT dan mulai menampilkan tampilan pengukuran.

---

## 🎮 6. Cara Penggunaan

Sistem menggunakan rotary encoder dan satu push button untuk mengontrol seluruh parameter osiloskop.

### Rotary Encoder

| Aksi | Fungsi |
|---|---|
| Putar | Mengubah nilai parameter yang dipilih |
| Tekan singkat | Berpindah parameter |
| Tahan 0.7 detik | Menjalankan AUTOSET |

Urutan parameter:

1. T/DIV
2. V/DIV
3. LEVEL
4. EDGE
5. MODE
6. CH
7. CTR

Parameter yang sedang aktif ditandai dengan garis bawah berwarna kuning.

### Push Button

| Aksi | Fungsi |
|---|---|
| Tekan singkat | RUN / HOLD |
| Tahan 0.7 detik | Mengganti halaman pengukuran |

### Pengaturan Parameter

**T/DIV — Time Division**

Mengatur rentang waktu horizontal pada layar.

Pilihan:

- 0.1 ms/div
- 0.2 ms/div
- 0.5 ms/div
- 1 ms/div
- 2 ms/div
- 5 ms/div
- 10 ms/div
- 20 ms/div
- 50 ms/div
- 100 ms/div

**V/DIV — Voltage Division**

Mengatur skala tegangan vertikal.

Pilihan:

- 0.05 V/div
- 0.1 V/div
- 0.2 V/div
- 0.5 V/div
- 1 V/div
- 2 V/div

**LEVEL — Trigger Level**

Menentukan tegangan ambang trigger.

**EDGE — Trigger Edge**

- RISE: Rising Edge.
- FALL: Falling Edge.

**MODE — Trigger Mode**

- AUTO: Tetap menampilkan waveform meskipun trigger tidak ditemukan.
- NORM: Menunggu trigger yang sesuai.
- SNGL: Mengambil satu capture ketika trigger ditemukan.

**CH — Channel Selection**

- CH1: GP26.
- CH2: GP27.
- CH1+CH2: Menampilkan kedua channel.

**CTR — Vertical Center**

Mengatur titik tengah tampilan vertikal.

Nilai AUTO dapat dipilih untuk menggunakan nilai rata-rata sinyal sebagai pusat tampilan.

---

## 📈 7. Sistem Visualisasi

Sistem menggunakan teknik **Min/Max Envelope Rendering**.

Berbeda dengan visualisasi yang hanya mengambil satu sampel untuk setiap kolom pixel, metode envelope menyimpan:

- Minimum sample.
- Maximum sample.
- First sample.
- Last sample.

Dengan demikian, perubahan sinyal yang terjadi di antara beberapa sampel tetap dapat terlihat pada layar.

Teknik ini berguna untuk mengamati sinyal yang memiliki ripple, noise, atau perubahan cepat.

### Double Buffering

Sistem menggunakan dua buffer:

- Capture Buffer: Menampung data ADC terbaru.
- Display Buffer: Menyimpan hasil capture sebelumnya untuk diproses dan ditampilkan.

Keduanya dipertukarkan setelah proses capture selesai.

Tujuannya adalah mengurangi gangguan antara proses sampling dan visualisasi.

---

## 📊 8. Halaman Pengukuran

Sistem memiliki tiga halaman pengukuran yang dapat diganti dengan menahan push button.

### Page 1 — Basic Measurement

- Vpp
- Average
- Frequency

### Page 2 — Voltage Measurement

- Maximum
- Minimum
- RMS

### Page 3 — Time Measurement

- Period
- Duty Cycle
- Gain (ketika CH1 dan CH2 aktif)

---

## ⚙️ 9. Konfigurasi Program

Pada bagian awal kode terdapat beberapa konfigurasi yang dapat diubah.

```python
CPU_MHZ    = 200
SPI_HZ     = 30_000_000
INVERT     = False
ENC_DIV    = 2
ENC_REV    = False
USE_DMA    = True
USE_WDT    = False
AUTO_RESET = False
LONG_MS    = 700
TRIG_HYST  = 25
```

| Parameter | Fungsi |
|---|---|
| CPU_MHZ | Frekuensi CPU |
| SPI_HZ | Kecepatan komunikasi TFT |
| INVERT | Inversi warna layar |
| ENC_DIV | Sensitivitas rotary encoder |
| ENC_REV | Membalik arah encoder |
| USE_DMA | Mengaktifkan DMA |
| USE_WDT | Mengaktifkan watchdog |
| AUTO_RESET | Restart otomatis saat error |
| LONG_MS | Durasi long press |
| TRIG_HYST | Histeresis trigger |

**Rekomendasi:** Pertahankan konfigurasi default terlebih dahulu. Setelah sistem stabil, lakukan pengujian dan optimasi satu per satu.

---

## ⚠️ 10. Catatan dan Batasan Pengukuran

Proyek ini merupakan osiloskop DIY untuk eksperimen dan pembelajaran elektronika, bukan pengganti osiloskop laboratorium yang telah dikalibrasi.

Beberapa hal yang perlu diperhatikan:

1. Akurasi tegangan bergantung pada referensi ADC dan kalibrasi aktual.
2. Input ADC Pico 2 tidak dirancang untuk menerima tegangan negatif secara langsung.
3. Pengukuran frekuensi membutuhkan sinyal dengan amplitudo dan kualitas trigger yang memadai.
4. Sampling aktual dapat berbeda dari nilai target.
5. Pada mode dua channel, bandwidth sampling per channel lebih rendah.
6. Tampilan envelope menunjukkan rentang sampel, bukan rekonstruksi analog sempurna.
7. Jangan mengukur langsung rangkaian listrik PLN atau tegangan tinggi menggunakan input ADC Pico 2.

---

## 🔧 11. Troubleshooting

| Masalah | Kemungkinan Penyebab |
|---|---|
| TFT tidak menyala | Wiring SPI atau konfigurasi display |
| Tampilan berantakan | Driver, orientasi, atau konfigurasi TFT |
| Waveform tidak muncul | Input ADC atau trigger |
| Trigger selalu WAIT | Level trigger tidak sesuai |
| Encoder terbalik | Atur `ENC_REV` |
| Sampling tidak stabil | CPU, DMA, atau konfigurasi clock |
| DMA gagal | Firmware atau konfigurasi register |
| Pembacaan tegangan tidak akurat | Referensi ADC atau kalibrasi |

Jika DMA mengalami kegagalan berulang, program memiliki mekanisme fallback ke software capture setelah tiga timeout.

---

## 🚀 12. Pengembangan Selanjutnya

Beberapa fitur yang berpotensi dikembangkan:

- [ ] Kalibrasi tegangan ADC.
- [ ] Input protection dan attenuator.
- [ ] Probe x1 dan x10.
- [ ] AC/DC coupling.
- [ ] Adjustable trigger hysteresis.
- [ ] Pengukuran fase antara CH1 dan CH2.
- [ ] FFT Spectrum Analyzer.
- [ ] Penyimpanan waveform ke file.
- [ ] Pengukuran otomatis lebih lengkap.
- [ ] Penambahan koneksi USB Serial untuk monitoring PC.

---

## 👨‍💻 13. Developer

**DIY Digital Oscilloscope Project**

Developed using:

- Raspberry Pi Pico 2 (RP2350)
- MicroPython
- DMA
- ADC
- TFT SPI
- Rotary Encoder

Proyek ini dikembangkan sebagai eksperimen embedded systems, digital signal acquisition, dan instrumentasi elektronika.

---

<p align="center">

**Built with curiosity, engineered with electronics.**

🔬 DIY Electronics | Embedded Systems | Signal Processing

</p>
