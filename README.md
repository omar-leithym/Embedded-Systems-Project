# Embedded-Systems-Project: ESP32 Robotic Hand with Pressure Sensing

A robotic hand that copies the movements of a glove worn by the user. Each glove
finger has a **flex sensor**. An **ESP32** reads the sensors and moves one **servo**
per robotic finger to match.

This project is based on Viral Science's
[Flex Sensor controlled Robo Hand](https://www.youtube.com/watch?v=Fvg-v8FPcjg),
with two main changes:

1. **ESP32 instead of Arduino Uno/Nano.** It has a 12-bit ADC and 3.3 V logic, and
   leaves room to add WiFi/Bluetooth control later.
2. **Pressure sensors (FSRs) on the robotic fingertips.** When a fingertip presses on
   an object past a set threshold, that finger stops closing. The hand can then hold
   things without crushing them or stalling the servos.

## Hardware

| Part | Qty |
|------|-----|
| ESP32 DevKit (38-pin or 30-pin) | 1 |
| Flex sensor (2.2" or 4.5") | 5 |
| Force-sensitive resistor (e.g. FSR402) | 5 |
| Servo (SG90 / MG90S) | 5 |
| 10 kΩ resistor (one per flex sensor and one per FSR) | 10 |
| External 5 V supply for the servos (≥ 2 A) | 1 |

## Wiring

| Finger | Flex sensor | Pressure sensor | Servo signal |
|--------|-------------|-----------------|--------------|
| Thumb  | GPIO 36 (VP) | GPIO 33 | GPIO 18 |
| Index  | GPIO 39 (VN) | GPIO 25 | GPIO 19 |
| Middle | GPIO 34 | GPIO 26 | GPIO 21 |
| Ring   | GPIO 35 | GPIO 27 | GPIO 22 |
| Pinky  | GPIO 32 | GPIO 14 | GPIO 23 |

- **Sensors**: wire each flex sensor and FSR as a voltage divider:
  `3.3V → sensor → GPIO → 10 kΩ → GND`.
  **Use 3.3 V, never 5 V.** ESP32 pins are not 5 V tolerant.
- **Servos**: power them from the external 5 V supply, **not** the ESP32's 3V3/5V
  pin. Connect the supply's GND to the ESP32's GND.
- GPIO 25/26/27/14 use ADC2, which does not work while WiFi is on. If you add WiFi
  later, move the pressure sensors to free ADC1 pins or use an external ADC
  (e.g. ADS1115).

## Software

- Arduino IDE with the **esp32 by Espressif Systems** board package
- **ESP32Servo** library by Kevin Harrington

The sketch is in [`RoboHand_ESP32/RoboHand_ESP32.ino`](RoboHand_ESP32/RoboHand_ESP32.ino).

## Calibration

Every flex sensor reads a little differently, so calibrate before use:

1. Leave `PRINT_RAW = true`, upload the sketch, and open Serial Monitor at **115200** baud.
2. Each line shows `flex/pressure` for the five fingers.
3. Write down the flex value for each finger with the hand **open** and in a **fist**.
   Put those numbers in `FLEX_OPEN[]` and `FLEX_CLOSED[]`.
4. Press each FSR the way an object would and set `PRESSURE_THRESHOLD[]` a little
   below that reading.
5. If a servo moves backwards, swap its `SERVO_OPEN` and `SERVO_CLOSED` values.
6. Set `PRINT_RAW = false` to see servo angles instead. A `*` marks a finger that is
   currently touching something.

## Credits

Original Arduino concept and code: [Viral Science](https://www.youtube.com/c/viralscience).
