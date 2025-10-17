// Revised Arduino Script (level_1_with_overflow.ino)
// Implements all I/O logic based on level28.py behavior

#include <NewPing.h>

// Pins configuration
#define RED_LED 9
#define GREEN_LED 8
#define DOOR_LOCK 22
#define PUMP_RELAY 24
#define ULTRASONIC_SMALL_TRIG 10
#define ULTRASONIC_SMALL_ECHO 11
#define ULTRASONIC_RES_TRIG 12
#define ULTRASONIC_RES_ECHO 13
#define DOOR_SENSOR_TOP 23
#define DOOR_SENSOR_GPIO2 7
#define DOOR_SENSOR_GPIO3 6
#define PUMP_GPIO2 25
#define DOOR_LOCK_2 26
#define DOOR_LOCK_3 27

// Constants
#define MAX_DISTANCE 200 // cm for ultrasonic
#define SMALL_TANK_THRESHOLD 8     // cm
#define RESERVOIR_HIGH 20          // cm
#define RESERVOIR_HIGH_HIGH 10     // cm
#define FINAL_WEIGHT_THRESHOLD 0.2 // kg

NewPing ultrasonicSmall(ULTRASONIC_SMALL_TRIG, ULTRASONIC_SMALL_ECHO, MAX_DISTANCE);
NewPing ultrasonicRes(ULTRASONIC_RES_TRIG, ULTRASONIC_RES_ECHO, MAX_DISTANCE);

// State variables
bool pouringActive = false;
bool transferInProgress = false;
bool doorOpenedSinceUnlock = false;


void setup() {
  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(DOOR_LOCK, OUTPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(DOOR_SENSOR_TOP, INPUT_PULLUP);
  pinMode(DOOR_SENSOR_GPIO2, INPUT_PULLUP);
  pinMode(DOOR_SENSOR_GPIO3, INPUT_PULLUP);
  pinMode(PUMP_GPIO2, OUTPUT);
  pinMode(DOOR_LOCK_2, OUTPUT);
  pinMode(DOOR_LOCK_3, OUTPUT);

  digitalWrite(RED_LED, HIGH);     // Standby
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(DOOR_LOCK, HIGH);   // Locked
  digitalWrite(PUMP_RELAY, HIGH);   // Off
  digitalWrite(PUMP_GPIO2, HIGH);
  digitalWrite(DOOR_LOCK_2, HIGH);
  digitalWrite(DOOR_LOCK_3, HIGH);
}

// Additional flag to track door-close-triggered start
bool doorJustClosed = false;
unsigned long doorClosedTime = 0;
bool doorOpenedFlag = false;


void loop() {
  // 1. Handle serial command from Pi
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if (command == "unlock") {
      digitalWrite(DOOR_LOCK, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      Serial.println("door_opened");
      doorOpenedFlag = true;
      pouringActive = true;
      doorOpenedSinceUnlock = false;  // Reset for new session

    } else if (command == "LOCK") {
      digitalWrite(PUMP_RELAY, HIGH);       // Stop pump
      digitalWrite(DOOR_LOCK, HIGH);        // Lock door
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      pouringActive = false;
      transferInProgress = false;

    } else if (command.startsWith("weight:")) {
      if (transferInProgress) {
        float weight = command.substring(7).toFloat();
        if (weight <= FINAL_WEIGHT_THRESHOLD) {
          digitalWrite(PUMP_RELAY, HIGH);
          Serial.println("transfer_done");
          transferInProgress = false;
          digitalWrite(GREEN_LED, LOW);
          digitalWrite(RED_LED, HIGH);
        }
      }
    }
  }

  // 2. Check sensors only if pouring is active
  if (pouringActive) {
    float smallDist = ultrasonicSmall.ping_cm();
    float resDist = ultrasonicRes.ping_cm();

    // Small tank overflow
    if (smallDist > 0 && smallDist <= SMALL_TANK_THRESHOLD) {
      Serial.println("overflow_small_tank");
      pouringActive = false;
      lockDoor();
    }

    // Reservoir overflow
    if (resDist > 0) {
      if (resDist <= RESERVOIR_HIGH_HIGH) {
        Serial.println("overflow_res_high_high");
        pouringActive = false;
        lockDoor();
      } else if (resDist <= RESERVOIR_HIGH) {
        Serial.println("overflow_res_high");
      }
    }

    // 3. Detect door activity
    if (!doorOpenedSinceUnlock) {
      // Wait for door to be opened at least once after unlock
      if (digitalRead(DOOR_SENSOR_TOP) == HIGH) {
        Serial.println("door_opened");
        doorOpenedFlag = true;
        doorOpenedSinceUnlock = true;
      }
    } else if (!transferInProgress && digitalRead(DOOR_SENSOR_TOP) == LOW) {
      // Door is closed → check if held closed for 4 seconds
      unsigned long doorCloseStart = millis();
      while (digitalRead(DOOR_SENSOR_TOP) == LOW) {
        if (millis() - doorCloseStart >= 4000) {
          Serial.println("door_closed");
          doorOpenedFlag = false;
          pouringActive = false;
          lockDoor();

          Serial.println("start_transfer");
          digitalWrite(PUMP_RELAY, LOW);  // Start pump
          transferInProgress = true;
          break;
        }
        delay(100); // debounce/hold check
      }
    }
  }

  delay(100);
}



void lockDoor() {
  digitalWrite(DOOR_LOCK, HIGH);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
}

void control_doors(float res_level, float small_tank_level) {
  if (res_level > 30 && small_tank_level > 15) {
    digitalWrite(DOOR_LOCK_2, LOW);
    digitalWrite(DOOR_LOCK_3, LOW);
    Serial.println("🚪 Doors 2 & 3 unlocked");
  } else {
    digitalWrite(DOOR_LOCK_2, HIGH);
    digitalWrite(DOOR_LOCK_3, HIGH);
    Serial.println("🔒 Doors 2 & 3 locked");
  }
}

void lock_door2() {
  if (digitalRead(DOOR_LOCK_2) == LOW) {
    while (digitalRead(DOOR_SENSOR_GPIO2) == HIGH) {
      Serial.println("⏳ Waiting Door 2 close...");
      delay(500);
    }
    digitalWrite(DOOR_LOCK_2, HIGH);
    Serial.println("🔒 Door 2 locked");
  } else if (digitalRead(DOOR_LOCK_3) == LOW) {
    while (digitalRead(DOOR_SENSOR_GPIO3) == HIGH) {
      Serial.println("⏳ Waiting Door 3 close...");
      delay(500);
    }
    digitalWrite(DOOR_LOCK_3, HIGH);
    Serial.println("🔒 Door 3 locked");
  }
}
