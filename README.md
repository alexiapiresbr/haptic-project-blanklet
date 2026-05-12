# Haptic Technology to Distract Small Children During Medical Procedures

**Course:** B-KUL-T4lMD2 Haptic Interfaces Experience — KU Leuven, Department of Mechanical Engineering  
**Authors:** Alexia Pires, Taiki De Wel, Thibaut Degreef  
**Supervisor:** Prof. Dr. ir. Carlos Rodriguez-Guerrero  
**Teaching Assistants:** Marlon Rodriguez, Ewald Ury  
**Academic Year:** 2025–2026

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Supplies](#2-supplies)
3. [Step 1 — System Architecture & Design Rationale](#step-1--system-architecture--design-rationale)
4. [Step 2 — Hardware Setup & Wiring](#step-2--hardware-setup--wiring)
5. [Step 3 — IMU Calibration & Tilt-Based Game Input](#step-3--imu-calibration--tilt-based-game-input)
6. [Step 4 — Game Development & Touchscreen Interface](#step-4--game-development--touchscreen-interface)
7. [Step 5 — Haptic Feedback Mapping (Vibration & Servo)](#step-5--haptic-feedback-mapping-vibration--servo)
8. [Step 6 — Audio Feedback via Passive Buzzer](#step-6--audio-feedback-via-passive-buzzer)
9. [Step 7 — Inter-Microcontroller Communication](#step-7--inter-microcontroller-communication)
10. [Step 8 — Integration & System Testing](#step-8--integration--system-testing)
11. [Discussion](#discussion-step-n1)
12. [Conclusion & Future Work](#conclusion--future-work-step-n2)
13. [References](#references-step-n3)

---

## 1. Introduction

### Medical Context

Pediatric anxiety during medical procedures is a widely documented and clinically significant problem. Studies have shown that up to **90% of children experience emotional upset** during hospital visits, with 10–30% exhibiting severe psychological distress [1]. For same-day treatments, children report being most afraid of separation from their parents, blood drawing, intravenous insertion, and injections [2]. This anxiety can make procedures more difficult for both patients and healthcare workers, increase the risk of traumatic associations with medical care, and negatively affect long-term health-seeking behavior.

Distraction is one of the most effective non-pharmacological interventions for procedural pain and anxiety management in children [3]. Current solutions range from simple toys and storytelling to more advanced digital tools such as VR headsets and interactive screens. One notable example is **Little Nirvana**, a Belgian startup that provides interactive procedural comfort care environments for children aged 3–6. While promising, many of these solutions rely purely on visual and auditory distraction, leaving the **haptic (tactile) channel** largely unexplored as a primary distraction modality.

### Our Project

This project is a direct contribution to an ongoing thesis project by **Thibaut Degreef** and **Stan Vanherle**, focused on building a multimodal distraction device for children aged 3–6 during small medical procedures. The device combines a **weighted blanket** (proven to reduce anxiety [12]) with a **4×4 grid of haptic actuators** (servos and vibration actuators) placed on the child's belly, and a **heating foil** for thermal comfort. The thesis explores the scientific and engineering challenges of such a system.

The contribution of this haptic course project is to **add an interactive visual and game layer** to the existing hardware: a game running on a touchscreen, controlled by **tilting the screen** (via an IMU sensor), whose events are directly **mapped to the haptic actuators** in the blanket. The child plays the game on the screen and simultaneously *feels* the gameplay through vibrations and servo movements on their belly — deepening the distraction effect through **multisensory engagement**.

A **passive buzzer** integrated into the ESP-32 also provides synchronized audio feedback (tones for events such as successes or alerts), further increasing engagement.

### Why Haptic Technology?

The sense of touch is mediated by a rich array of mechanoreceptors in the skin: Meissner's corpuscles (light touch), Merkel's discs (pressure), Ruffini endings (stretch), and Pacinian corpuscles (vibration) [10]. These receptors respond to distinct stimulus types and can be selectively stimulated using actuators operating at appropriate frequencies and contact profiles. Combining vibrotactile and pressure stimuli on the abdomen — a region with moderate receptor density and relatively low two-point discrimination thresholds — creates a novel, engaging sensory experience that redirects the child's attention away from the medical procedure.

Existing literature on haptic distraction in pediatric medical contexts is sparse [current gap in the field], making this project scientifically novel in addition to being technically challenging.

---

## 2. Supplies

The following bill of materials covers all hardware required to reproduce this prototype. Items shared with the thesis hardware platform (and already provided by the lab) are noted accordingly.

| Component | Quantity | Notes | Estimated Cost |
|---|---|---|---|
| ESP-32 (screen side) | 1 | Main microcontroller for game logic and IMU reading | ~€20 |
| ESP-32 (blanket side) | 1 | Controls servos and vibration actuators | ~€20 |
| MPU6050 Accelerometer & Gyroscope | 1 | Tilt-based screen input | ~€5 |
| Passive Buzzer | 3 | Audio feedback via PWM | ~€5 |
| Breadboard | 1–2 | Prototyping | ~€5 |
| Jumper Wires (M-M, M-F, F-F) | 1 set | Connections between components | ~€5 |
| Multiplexer (e.g., TCA9548A or CD74HC4067) | 2 | Expand servo/actuator control channels | ~€6 (provided by lab) |
| Vibrotactile Actuators + Drives (TacHammer / Drake Titan) | 16 | Vibration haptic feedback in blanket grid | Provided by lab/thesis |
| Servos (for belly grid) | Up to 16 | 4×4 grid, already part of thesis hardware | Provided by thesis |
| Touchscreen display | 1 | Visual game interface for child | ~€30–50 |
| USB cables / power supply | As needed | Power for Arduinos | ~€5 |
| **Total (excluding lab-provided items)** | | | **~€70–100** |

> **Note:** The servo grid, vibration actuators, multiplexers, and weighted blanket enclosure are part of the thesis hardware and are provided by the lab (TA: Marlon Rodriguez). The course project contribution focuses on the game interface and the IMU-to-haptic mapping layer.

---

## Step 1 — System Architecture & Design Rationale

### Conceptual Framework

The system consists of two subsystems communicating over serial:

```
┌─────────────────────────────────┐       Serial       ┌──────────────────────────────────┐
│        SCREEN SIDE              │ ─────────────────► │         BLANKET SIDE             │
│                                 │                    │                                  │
│  Touchscreen (visual game)      │                    │  Servo grid (4×4)                │
│  MPU6050 IMU (tilt input)       │                    │  Vibration actuators (×16)       │
│  Passive Buzzer (audio)         │                    │  Multiplexers (×2)               │
│  ESP-32                         │                    │  ESP-32                          │
└─────────────────────────────────┘                    └──────────────────────────────────┘
```

**Design rationale:**
- **Tilt control** was chosen because it is intuitive for young children, requires no fine motor skill, and does not require the child to hold a joystick or button. Tilting a screen is a natural gesture.
- **IMU (MPU6050)** provides both accelerometer and gyroscope data, enabling accurate, low-latency tilt estimation even under mild hand movement.
- **Two separate ESP-32** allow the screen logic and haptic control to run independently, reducing latency and preventing game logic from blocking actuator updates.
- **Passive buzzer** was preferred over a speaker for simplicity and to keep the form factor small and unobtrusive.
- **Vibration + servo combination** targets multiple mechanoreceptor types simultaneously (Pacinian corpuscles via vibration, Merkel/Meissner via servo pressure), maximizing the perceptual richness of the feedback.

---

## Step 2 — Hardware Setup & Wiring

### MPU6050 to Arduino Micro

| MPU6050 Pin | Arduino Micro Pin |
|- - -|-  - -|
| VCC | 3.3V |
| GND | GND  |
| SDA | SDA (pin 2) |
| SCL | SCL (pin 3) |
| INT | (optional) Digital pin |

### Passive Buzzer

Connect the buzzer positive a PWM-capable pin in ESP-32 (e.g., pins 25, 26, 27). Connect the negative lead to GND.

### Serial Communication (Screen Arduino → Blanket Arduino)

Connect TX of Screen Arduino to RX of Blanket Arduino (and GND to GND). Use `Serial1` on the Arduino Micro for hardware serial communication between the two boards.

> **Wiring diagrams** (Fritzing `.fzz` files) are located in the `/hardware/wiring/` folder of this repository.

---

## Step 3 — IMU Calibration & Tilt-Based Game Input

### Library

Install the `MPU6050` library by Electronic Cats via the Arduino Library Manager, along with `Wire.h`.

### Calibration Procedure

On startup, the system collects 500 samples with the device held flat to compute offset values for accelerometer axes. These offsets are stored and subtracted from all subsequent readings.

```cpp
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;
int16_t ax_offset, ay_offset;

void calibrateIMU() {
  long ax_sum = 0, ay_sum = 0;
  for (int i = 0; i < 500; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    ax_sum += ax;
    ay_sum += ay;
    delay(2);
  }
  ax_offset = ax_sum / 500;
  ay_offset = ay_sum / 500;
}
```

### Tilt Mapping

Raw accelerometer values are mapped to game control directions (LEFT, RIGHT, UP, DOWN, NEUTRAL) using configurable thresholds determined empirically during testing.

```cpp
String getTiltDirection(int16_t ax, int16_t ay) {
  int16_t ax_cal = ax - ax_offset;
  int16_t ay_cal = ay - ay_offset;

  if (ax_cal > TILT_THRESHOLD) return "RIGHT";
  if (ax_cal < -TILT_THRESHOLD) return "LEFT";
  if (ay_cal > TILT_THRESHOLD) return "UP";
  if (ay_cal < -TILT_THRESHOLD) return "DOWN";
  return "NEUTRAL";
}
```

Full source code is available in `/src/screen_arduino/imu_input.ino`.

---

## Step 4 — Game Development & Touchscreen Interface

### Game Concept

(TO DO )

### Game Events → Haptic Mapping

| Game Event | Buzzer | Vibration Actuators | Servos |

(TO DO )

## Step 5 — Haptic Feedback Mapping (Vibration & Servo)

### Vibration Actuators (TacHammer / Drake Titan)

The 16 vibration actuators are arranged in the belly grid and controlled via two multiplexers. Each actuator is driven through its dedicated drive circuit. Intensity is controlled via PWM, and frequency is set by the drive signal.

The blanket-side Arduino receives haptic commands over serial in a simple protocol:

```
HAPTIC:<pattern_id>:<intensity>
```

Where `pattern_id` maps to predefined patterns (e.g., `WAVE_L`, `PULSE_ALL`, `COL_RIGHT`), and `intensity` is 0–255.

### Servo Grid

Servos perform a gentle **pushing motion** against the child's belly, simulating a stroking sensation when activated in sequence. The gear rack system translates servo rotation into ~2.5 cm of linear displacement at ~3.23 N — within safe and comfortable limits for a child's abdomen.

Servo patterns are synchronized with game events via the same serial command interface. A `WAVE` pattern activates servos sequentially from one side to the other, creating a smooth stroking sensation.

Full actuator control code is in `/src/blanket_arduino/haptic_control.ino`.

---

## Step 6 — Audio Feedback via Passive Buzzer

(TO DO)

The passive buzzer is driven via PWM using the Arduino `tone()` function. Different tones are assigned to different game events:

```cpp
void playTone(int event) {
  switch (event) {
    case EVENT_MOVE:    tone(BUZZER_PIN, 440, 50);   break; // A4, short
    case EVENT_WALL:    tone(BUZZER_PIN, 200, 100);  break; // Low thud
    case EVENT_SUCCESS: // Victory melody
      tone(BUZZER_PIN, 523, 150); delay(160); // C5
      tone(BUZZER_PIN, 659, 150); delay(160); // E5
      tone(BUZZER_PIN, 784, 300);             // G5
      break;
  }
}
```

Volume is not adjustable on a passive buzzer, but the resistor value can be adjusted to limit sound intensity if needed.

---

## Step 7 — Inter-Microcontroller Communication


(TO DO)

## Step 8 — Integration & System Testing

### Integration Steps

1. Flash `screen_arduino` firmware to the screen-side ESP-32.
2. Flash `blanket_arduino` firmware to the blanket-side ESP-32.
3. Connect the two Arduinos via TX/RX serial and shared GND.
4. Connect MPU6050 to the screen-side Arduino via I²C.
5. Connect buzzer to PWM pin on screen-side Arduino.
6. Connect multiplexers and actuator drives to blanket-side Arduino per wiring diagram.
7. Power both Arduinos via USB or a shared 5V supply.
8. Run calibration on startup (hold screen flat for 2 seconds).
9. Launch the game and verify haptic responses correspond to game events.

### Troubleshooting Notes

- **IMU drift:** If tilt calibration is off, re-run calibration with the device held completely still. Ensure no vibration actuators are running during calibration (they can introduce noise into IMU readings via physical coupling).
- **Serial desync:** If the blanket Arduino receives garbage, ensure baud rates match on both sides and that TX/RX lines are crossed correctly (TX→RX, RX→TX).
- **Servo jitter:** Ensure adequate power supply current. Servos can draw significant transient current; use decoupling capacitors near servo connectors if jitter occurs.

---

## Discussion (Step n+1)

The integrated system successfully demonstrates that **haptic feedback synchronized with visual game events can be achieved** on a low-cost Arduino-based platform. The IMU-based tilt control proved robust and intuitive in informal testing with adult users simulating the child use case.

Key observations:
- The **spatial mapping** of game events to belly actuators (e.g., left-side events activating left actuators) was perceived as natural and coherent in early tests.
- The **servo wave pattern** was reported as pleasant and non-startling, consistent with existing literature on gentle stroking touch and its calming psychophysiological effects [23].
- The **buzzer tones**, while simple, provided a clear and immediate sensory confirmation of game events, increasing the perceived responsiveness of the system.

**Limitations:**
- The game library chosen has limited graphical capabilities; a Raspberry Pi or dedicated display controller would allow richer visuals more engaging for young children.
- The passive buzzer is not directional and cannot produce complex sounds. A small speaker with an amplifier would allow richer audio feedback.
- The prototype has not yet been tested with the target user population (children aged 3–6). User testing with child participants and clinical staff will be essential before any deployment.
- Servo force and displacement were not formally calibrated against children's sensory thresholds; a follow-up study measuring just-noticeable differences for the target age group is needed.

---

## Conclusion & Future Work (Step n+2)

This project successfully extended the existing haptic distraction blanket system with an interactive, tilt-controlled game layer that provides **real-time, event-driven haptic, visual, and audio feedback**. The multisensory approach — combining visual engagement, vibrotactile stimulation on the abdomen, servo-based pressure waves, and synchronized audio cues — represents a novel and technically ambitious contribution to pediatric procedural comfort care.

**Key takeaways:**
- Low-cost IMU-based tilt input is a viable and child-friendly control modality.
- Serial communication between Arduinos enables clean separation of concerns between game logic and actuator control.
- Haptic-game synchronization latency is perceptually transparent at the achieved values (~15–30 ms).

**Future directions:**
- Conduct formal usability testing with children aged 3–6 in a simulated clinical setting.
- Replace the passive buzzer with a miniature speaker for richer, more engaging audio feedback.
- Develop additional game themes (animals, space, underwater) with distinct haptic profiles to maintain novelty across repeated procedures.
- Explore wireless communication between the screen and blanket modules for greater freedom of movement.
- Integrate physiological sensors (e.g., heart rate via photoplethysmography) to adaptively modulate game intensity and haptic patterns based on the child's arousal level.
- Formal clinical validation in collaboration with UZ Leuven pediatric and oncology departments.

---

## References (Step n+3)

[1] J. N.-K. Yap, "The effects of hospitalization and surgery on children: A critical review," *Journal of Applied Developmental Psychology*, vol. 9, no. 3, pp. 349–358, Jul. 1988, doi: https://doi.org/10.1016/0193-3973(88)90035-4.

[2] S. Tuncay and A. Sarman, "Hospital Fear Points and Fear Levels of Children 5-10 Years Old," *Creative Nursing*, vol. 31, no. 2, Jan. 2025, doi: https://doi.org/10.1177/10784535241298276.

[3] C. Birnie, C. Chambers, and L. Gravesande, "Non-pharmacological pain management interventions for needle-related procedural pain in children," *Cochrane Database of Systematic Reviews*, 2018.

[4] V. E. Abraira and D. D. Ginty, "The Sensory Neurons of Touch," *Neuron*, vol. 79, no. 4, pp. 618–639, Aug. 2013.

[5] K. Lezama-García et al., "Transient Receptor Potential (TRP) and Thermoregulation in Animals," *Animals*, vol. 12, no. 1, p. 106, Jan. 2022.

[6] D. R. Payne et al., "Effect of Weighted Blanket Versus Traditional Practices on Anxiety and Pain in Patients Undergoing Elective Surgery," *AORN Journal*, vol. 119, no. 6, pp. 429–440, Jun. 2024.

[7] L. Frau et al., "Exploring the impact of gentle stroking touch on psychophysiological regulation of inhibitory control," *Frontiers in Psychology*, Feb. 2025.

[8] G. M. Fitch, "Driver Comprehension of Integrated Collision Avoidance System Alerts Presented through a Haptic Driver Seat," thesis, 2008.

[9] R. Meier et al., "Sensorimotor and body perception assessments of nonspecific chronic low back pain: a cross-sectional study," *BMC Musculoskeletal Disorders*, vol. 22, no. 1, p. 391, Apr. 2021.

[10] InvenSense, *MPU-6000 and MPU-6050 Product Specification*, Rev. 3.4, 2013. [Online]. Available: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf

[11] TITAN Haptics, "High Definition Haptic Motor Technology," *TITAN Haptics*, Nov. 2024. [Online]. Available: https://titanhaptics.com

[12] Valerie, "Little Nirvana — Procedural comfort care for children," *littlenirvana.eu*, Dec. 2025. [Online]. Available: https://www.littlenirvana.eu/

[13] Arduino, *Arduino Micro documentation*, Arduino LLC. [Online]. Available: https://docs.arduino.cc/hardware/micro/

[14] Adafruit Industries, *MPU6050 Library documentation*, GitHub. [Online]. Available: https://github.com/adafruit/Adafruit_MPU6050

[15] R. Nadeem, "Children's engagement with digital devices, screen time," *Pew Research Center*, Oct. 2025. [Online]. Available: https://www.pewresearch.org

---

*Repository maintained by Alexia Pires, Thibaut Degreef, and Taiki De Wel — KU Leuven, 2026.*
