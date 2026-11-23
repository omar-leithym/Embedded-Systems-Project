// Flex-sensor controlled robotic hand with fingertip pressure feedback - ESP32 port
// Based on the Arduino "Flex Sensor controlled Robo Hand" by Viral Science
// (www.viralsciencecreativity.com, https://www.youtube.com/watch?v=Fvg-v8FPcjg)
//
// What's different from the Arduino original:
//   * Runs on an ESP32 (12-bit ADC, 0-4095, 3.3 V logic) using the ESP32Servo library
//   * Adds a force-sensitive resistor (FSR) on each robotic fingertip. When a finger
//     touches something hard enough, that servo stops closing further, so the hand
//     doesn't crush objects or stall the servo.
//   * Fixes the constrain(x, 180, 0) bug in the original (min must be <= max)
//
// Required library: "ESP32Servo" by Kevin Harrington (Arduino Library Manager)

#include <ESP32Servo.h>

const int NUM_FINGERS = 5;   // thumb, index, middle, ring, pinky

// ---------------- Pin assignment ----------------
// Flex sensors on ADC1 pins (input-only, always safe to use).
const int FLEX_PINS[NUM_FINGERS]     = {36, 39, 34, 35, 32};
// Pressure sensors (FSR). 33 is ADC1; 25/26/27/14 are ADC2.
// NOTE: ADC2 pins stop working if WiFi is turned on. This sketch doesn't use WiFi.
const int PRESSURE_PINS[NUM_FINGERS] = {33, 25, 26, 27, 14};
// Servo signal pins (any PWM-capable output pin).
const int SERVO_PINS[NUM_FINGERS]    = {18, 19, 21, 22, 23};

// ---------------- Calibration ----------------
// Raw ADC readings with your glove hand fully OPEN and fully CLOSED (fist).
// Get these by setting PRINT_RAW = true, opening Serial Monitor at 115200 baud,
// and writing down the numbers for each finger.
// It's fine if OPEN > CLOSED for some fingers, map() handles both directions.
int FLEX_OPEN[NUM_FINGERS]   = {2200, 2200, 2200, 2200, 2200};
int FLEX_CLOSED[NUM_FINGERS] = {2900, 2900, 2900, 2900, 2900};

// Servo angle for an open and a closed robotic finger. Swap per finger
// if a servo is mounted mirrored.
int SERVO_OPEN[NUM_FINGERS]   = {0, 0, 0, 0, 0};
int SERVO_CLOSED[NUM_FINGERS] = {180, 180, 180, 180, 180};

// Raw FSR reading above which we say the fingertip is "touching" something.
// Higher means more force is needed. Tune using PRINT_RAW output.
int PRESSURE_THRESHOLD[NUM_FINGERS] = {1500, 1500, 1500, 1500, 1500};

const bool PRINT_RAW = true;         // true: print raw sensor values; false: print angles
const int  LOOP_DELAY_MS = 20;       // ~50 Hz update (original used 300 ms)
const float SMOOTHING = 0.3;         // 0..1, lower = smoother but slower response

// ---------------- State ----------------
Servo servos[NUM_FINGERS];
float smoothedFlex[NUM_FINGERS];
int   currentAngle[NUM_FINGERS];

// Returns true if the target angle moves the finger further toward "closed"
// than where it is now.
bool isClosingFurther(int finger, int target) {
  if (SERVO_CLOSED[finger] > SERVO_OPEN[finger]) {
    return target > currentAngle[finger];
  }
  return target < currentAngle[finger];
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);              // 0-4095
  analogSetAttenuation(ADC_11db);        // full 0-3.3 V input range

  // ESP32Servo needs hardware timers allocated before attaching.
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  for (int i = 0; i < NUM_FINGERS; i++) {
    pinMode(FLEX_PINS[i], INPUT);
    pinMode(PRESSURE_PINS[i], INPUT);

    servos[i].setPeriodHertz(50);                // standard hobby servo
    servos[i].attach(SERVO_PINS[i], 500, 2400);  // pulse width range in us (SG90/MG90S)

    smoothedFlex[i] = analogRead(FLEX_PINS[i]);
    currentAngle[i] = SERVO_OPEN[i];
    servos[i].write(currentAngle[i]);
  }

  Serial.println("RoboHand ESP32 ready");
}

void loop() {
  int flexRaw[NUM_FINGERS];
  int pressureRaw[NUM_FINGERS];
  bool touching[NUM_FINGERS];

  for (int i = 0; i < NUM_FINGERS; i++) {
    // 1. Read and smooth the flex sensor
    flexRaw[i] = analogRead(FLEX_PINS[i]);
    smoothedFlex[i] = SMOOTHING * flexRaw[i] + (1.0 - SMOOTHING) * smoothedFlex[i];

    // 2. Convert flex reading to a servo angle
    int target = map((int)smoothedFlex[i],
                     FLEX_OPEN[i], FLEX_CLOSED[i],
                     SERVO_OPEN[i], SERVO_CLOSED[i]);
    target = constrain(target,
                       min(SERVO_OPEN[i], SERVO_CLOSED[i]),
                       max(SERVO_OPEN[i], SERVO_CLOSED[i]));

    // 3. Pressure feedback: if the fingertip is pressing on something,
    //    don't let it close any further (opening is always allowed).
    pressureRaw[i] = analogRead(PRESSURE_PINS[i]);
    touching[i] = pressureRaw[i] > PRESSURE_THRESHOLD[i];

    if (!(touching[i] && isClosingFurther(i, target))) {
      currentAngle[i] = target;
    }
    servos[i].write(currentAngle[i]);
  }

  // 4. Debug output. Format per finger:  flex/pressure  or  angle[*]  (* = touching)
  for (int i = 0; i < NUM_FINGERS; i++) {
    if (PRINT_RAW) {
      Serial.print(flexRaw[i]);
      Serial.print("/");
      Serial.print(pressureRaw[i]);
    } else {
      Serial.print(currentAngle[i]);
      if (touching[i]) Serial.print("*");
    }
    Serial.print(i < NUM_FINGERS - 1 ? " -- " : "\n");
  }

  delay(LOOP_DELAY_MS);
}
