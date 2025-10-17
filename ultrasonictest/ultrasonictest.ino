// Show ultrasonic readings in Serial Monitor (115200)
// Your pin map:
#define ULTRASONIC_SMALL_TRIG 10
#define ULTRASONIC_SMALL_ECHO 11
#define ULTRASONIC_RES_TRIG   12
#define ULTRASONIC_RES_ECHO   13   // ok as INPUT

static float read_cm(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long dur = pulseIn(echo, HIGH, 30000UL); // timeout 30 ms (~5 m)
  return dur ? (dur * 0.0343f / 2.0f) : -1.0f;      // cm
}

void setup() {
  Serial.begin(115200);                // 1) start serial
  pinMode(ULTRASONIC_SMALL_TRIG, OUTPUT);
  pinMode(ULTRASONIC_SMALL_ECHO, INPUT);
  pinMode(ULTRASONIC_RES_TRIG,   OUTPUT);
  pinMode(ULTRASONIC_RES_ECHO,   INPUT);
  Serial.println("Ultrasonic test: small(T10/E11), res(T12/E13)");
}

void loop() {
  float small = read_cm(ULTRASONIC_SMALL_TRIG, ULTRASONIC_SMALL_ECHO);
  float res   = read_cm(ULTRASONIC_RES_TRIG,   ULTRASONIC_RES_ECHO);

  // 2) print nicely
  Serial.print("Small: ");
  if (small < 0) Serial.print("No echo");
  else { Serial.print(small, 1); Serial.print(" cm"); }

  Serial.print(" | Res: ");
  if (res < 0) Serial.println("No echo");
  else { Serial.print(res, 1); Serial.println(" cm"); }

  delay(200);
}
