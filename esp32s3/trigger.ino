
/*
  MINI OSCILOSKOP ESP32-S3
  Input analog : GPIO4 (ADC1_CH3)
  Output      : Serial Plotter Arduino IDE

  Uji dengan sinyal aman 0-3.3V.
*/

#include <Arduino.h>

const int ADC_PIN = 4;

const int SAMPLE_COUNT = 400;
const uint32_t SAMPLE_INTERVAL_US = 20; // Target interval 20 us

uint16_t samples[SAMPLE_COUNT];

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // Nilai ADC 0-4095
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  delay(1000);
  Serial.println("ESP32-S3 Mini Oscilloscope");
  delay(1000);
}

void captureSignal() {
  uint32_t nextSample = micros();

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    while ((int32_t)(micros() - nextSample) < 0) {
      // Tunggu jadwal sampling
    }

    samples[i] = analogRead(ADC_PIN);
    nextSample += SAMPLE_INTERVAL_US;
  }
}

void displaySignal() {
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    // Konversi perkiraan ADC menjadi tegangan
    float voltage = samples[i] * 3.3f / 4095.0f;

    // Format yang bisa dibaca Serial Plotter
    Serial.println(voltage, 3);
  }
}

void loop() {
  captureSignal();
  displaySignal();

  delay(20);
}
