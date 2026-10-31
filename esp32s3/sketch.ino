/*
  ESP32-S3 SIGNAL GENERATOR (untuk tes osiloskop Pico)

  GPIO 4  = 1 kHz square 50%        (hardware PWM, tanpa jitter)
  GPIO 5  = 500 Hz PWM 25%          (hardware PWM)
  GPIO 6  = Pulse 20 Hz             (ubah PULSE_HZ kalau mau)
  GPIO 7  = 100 Hz square 50%
  GPIO 15 = SINUS dari PWM 20 kHz   <-- sinyal demo kapasitor 104
            tombol BOOT (GPIO 0):
              tekan singkat = ganti AMPLITUDO  (3.3 / 1.65 / 0.83 / 0.33 Vpp)
              tahan >0.7 dtk = ganti FREKUENSI (25 / 50 / 100 / 200 Hz)

  Wajib Arduino-ESP32 core 3.x (memakai ledcAttachChannel)
*/

#define PIN_SQUARE 4
#define PIN_PWM    5
#define PIN_PULSE  6
#define PIN_TEST   7
#define PIN_SINE   15
#define PIN_BTN    0

const uint32_t PULSE_HZ = 20;    // 2 Hz tidak terlihat di scope (layar max 100 ms)
const uint32_t TEST_HZ  = 100;

// ---------- sinus dari PWM ----------
const int   N          = 64;                       // titik per siklus
const int   SINE_BITS  = 10;                       // resolusi PWM 10 bit
const int   SINE_MID   = 512;
const float AMP_LIST[] = {1.0f, 0.5f, 0.25f, 0.1f};
const uint16_t FRQ_LIST[] = {25, 50, 100, 200};

int16_t  sineTab[N];
int      ampIdx = 0;
int      frqIdx = 1;                // default 50 Hz
int      sineIdx = 0;
uint32_t sineStepUs = 0;

// ---------- timer software ----------
uint32_t nextSine, nextPulse, nextTest;
bool statePulse = false, stateTest = false;

inline bool due(uint32_t now, uint32_t next) {
  return (int32_t)(now - next) >= 0;     // aman saat micros() overflow
}

void updateSineStep() {
  sineStepUs = 1000000UL / ((uint32_t)FRQ_LIST[frqIdx] * N);
}

void printStatus() {
  Serial.printf("Sinus GPIO15: %u Hz, %.2f Vpp\n",
                FRQ_LIST[frqIdx], 3.3f * AMP_LIST[ampIdx]);
}

void handleButton() {
  static bool lastState = HIGH;
  static uint32_t t0 = 0;
  static bool longDone = false;

  bool s = digitalRead(PIN_BTN);
  uint32_t ms = millis();

  if (lastState == HIGH && s == LOW) {          // baru ditekan
    t0 = ms;
    longDone = false;
  }
  if (s == LOW && !longDone && ms - t0 >= 700) { // ditahan -> ganti frekuensi
    longDone = true;
    frqIdx = (frqIdx + 1) % 4;
    updateSineStep();
    printStatus();
  }
  if (lastState == LOW && s == HIGH && !longDone && ms - t0 > 30) {  // lepas -> ganti amplitudo
    ampIdx = (ampIdx + 1) % 4;
    printStatus();
  }
  lastState = s;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_TEST, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);

  // Channel genap (0,2,4) -> tiap sinyal punya timer sendiri
  ledcAttachChannel(PIN_SQUARE, 1000, 8, 0);   ledcWrite(PIN_SQUARE, 128);  // 50%
  ledcAttachChannel(PIN_PWM,     500, 8, 2);   ledcWrite(PIN_PWM,     64);  // 25%
  ledcAttachChannel(PIN_SINE, 60000, SINE_BITS, 4);   // dulu 20000
  ledcWrite(PIN_SINE, SINE_MID);

  for (int i = 0; i < N; i++)
    sineTab[i] = (int16_t)roundf(511.0f * sinf(2.0f * PI * i / N));

  updateSineStep();

  uint32_t now = micros();
  nextSine = nextPulse = nextTest = now;

  Serial.println("ESP32-S3 SIGNAL GENERATOR");
  Serial.println("GPIO 4  : Square 1 kHz");
  Serial.println("GPIO 5  : PWM 500 Hz / 25%");
  Serial.printf ("GPIO 6  : Pulse %u Hz\n", (unsigned)PULSE_HZ);
  Serial.printf ("GPIO 7  : Square %u Hz\n", (unsigned)TEST_HZ);
  Serial.println("GPIO 15 : Sinus (PWM 20 kHz) - BOOT: amplitudo / tahan: frekuensi");
  printStatus();
}

void loop() {
  uint32_t now = micros();

  // Sinus: update duty PWM tiap sineStepUs
  if (due(now, nextSine)) {
    nextSine += sineStepUs;
    int duty = SINE_MID + (int)(sineTab[sineIdx] * AMP_LIST[ampIdx]);
    ledcWrite(PIN_SINE, duty);
    if (++sineIdx >= N) sineIdx = 0;
  }

  // Pulse
  if (due(now, nextPulse)) {
    nextPulse += 1000000UL / (2UL * PULSE_HZ);
    statePulse = !statePulse;
    digitalWrite(PIN_PULSE, statePulse);
  }

  // Square test
  if (due(now, nextTest)) {
    nextTest += 1000000UL / (2UL * TEST_HZ);
    stateTest = !stateTest;
    digitalWrite(PIN_TEST, stateTest);
  }

  handleButton();
}
