
/*
  ESP32-S3 MINI SIGNAL GENERATOR
  Output:
  GPIO 4 = 1 kHz Square Wave
  GPIO 5 = 500 Hz PWM, 25%
  GPIO 6 = 2 Hz Pulse
  GPIO 7 = 100 Hz Square Wave
*/

#define PIN_SQUARE 4
#define PIN_PWM    5
#define PIN_PULSE  6
#define PIN_TEST   7

unsigned long lastSquare = 0;
unsigned long lastPulse = 0;
unsigned long lastTest  = 0;

bool stateSquare = false;
bool statePulse  = false;
bool stateTest   = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SQUARE, OUTPUT);
  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_TEST, OUTPUT);

  // PWM 500 Hz, resolusi 8 bit
  ledcAttach(PIN_PWM, 500, 8);
  ledcWrite(PIN_PWM, 64); // 25%

  Serial.println("ESP32-S3 SIGNAL GENERATOR");
  Serial.println("GPIO 4 : Square 1 kHz");
  Serial.println("GPIO 5 : PWM 500 Hz / 25%");
  Serial.println("GPIO 6 : Pulse 2 Hz");
  Serial.println("GPIO 7 : Square 100 Hz");
}

void loop() {
  unsigned long now = micros();

  // GPIO 4: 1 kHz
  if (now - lastSquare >= 500) {
    lastSquare = now;
    stateSquare = !stateSquare;
    digitalWrite(PIN_SQUARE, stateSquare);
  }

  // GPIO 6: 2 Hz
  if (now - lastPulse >= 250000) {
    lastPulse = now;
    statePulse = !statePulse;
    digitalWrite(PIN_PULSE, statePulse);
  }

  // GPIO 7: 100 Hz
  if (now - lastTest >= 5000) {
    lastTest = now;
    stateTest = !stateTest;
    digitalWrite(PIN_TEST, stateTest);
  }
}
