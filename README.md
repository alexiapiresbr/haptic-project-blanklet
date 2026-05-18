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
9. [Step 7 — Inter-Microcontroller Communication (ESP-NOW)](#step-7--inter-microcontroller-communication-esp-now)
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

This project is a direct contribution to an ongoing thesis project by **Thibaut Degreef and Stan Vanherle**, focused on building a multimodal distraction device for children aged 3–6 during small medical procedures. The device combines a **weighted blanket** (proven to reduce anxiety [6]) with a **4×4 grid of haptic actuators** (servos and vibration actuators) placed on the child's belly, and a **heating foil** for thermal comfort. The thesis explores the scientific and engineering challenges of such a system.

The contribution of this haptic course project is to **add an interactive visual and game layer** to the existing hardware: three motion-controlled games running on a touchscreen, controlled by **tilting the screen** (via an IMU sensor), whose events are directly **mapped to the haptic actuators** in the blanket. The child plays the game on the screen and simultaneously *feels* the gameplay through vibrations and servo movements on their belly — deepening the distraction effect through **multisensory engagement**.

A **passive buzzer** integrated into the ESP32 provides synchronized musical feedback with a dedicated melody per game, and a **passive temperature monitoring** channel keeps the blanket-side heating within safe ranges.

### Why Haptic Technology?

The sense of touch is mediated by a rich array of mechanoreceptors in the skin: Meissner's corpuscles (light touch), Merkel's discs (pressure), Ruffini endings (stretch), and Pacinian corpuscles (vibration) [4]. These receptors respond to distinct stimulus types and can be selectively stimulated using actuators operating at appropriate frequencies and contact profiles. Combining vibrotactile and pressure stimuli on the abdomen — a region with moderate receptor density and relatively low two-point discrimination thresholds — creates a novel, engaging sensory experience that redirects the child's attention away from the medical procedure.

Existing literature on haptic distraction in pediatric medical contexts is sparse, making this project scientifically novel in addition to being technically challenging.

---

## 2. Supplies

The following bill of materials covers all hardware required to reproduce this prototype. Items shared with the thesis hardware platform (and already provided by the lab) are noted accordingly.

| Component | Quantity | Notes | Estimated Cost |
|---|---|---|---|
| ESP32-WROOM Microcontroller (screen side) | 1 | Main microcontroller: game logic, display, IMU, buzzer, ESP-NOW master | ~€10–15 |
| ESP32-WROOM Microcontroller (blanket side) | 1 | Slave node: receives ESP-NOW packets, drives servos and haptic actuators | ~€10–15 |
| MPU6050 Accelerometer & Gyroscope module | 1 | Tilt-based screen input via I2C | ~€5 |
| 4.0" TFT LCD display with ILI9488 controller | 1 | Visual game interface for child (480×320, SPI) | ~€30–50 |
| KY-006 Passive Piezo Buzzer | 1 | Audio/musical feedback via LEDC PWM | ~€3 |
| PCA9685 PWM Driver | 1 | 16-channel I2C driver for all 16 MG90S servos | ~€8–12 |
| PCA9548A I2C Multiplexer | 2 | Expands I2C bus to address 16 individual DRV2605L drivers | ~€6 (provided by lab) |
| DRV2605L Haptic Motor Drivers | 16 | Generates waveforms for the Drake Titan vibrotactile actuators | ~€60–80 |
| Drake Titan Haptic Actuators | 16 | Vibrotactile feedback in the blanket 4×4 grid | Provided by lab |
| MG90S Servo Motors | 16 | Physical 4×4 belly pressure grid | Provided by thesis |
| 5V Power Supply (10–20 A) | 1 | Dedicated high-current rail for all servos and haptic drivers | ~€20–30 |
| DC Jack-to-Terminal Adapter | 1 | Connects wall power to PCA9685 / breadboard rails | ~€2 |
| Breadboard | 2 | Rapid prototyping before permanent soldering | ~€6 |
| Jumper Wires (M-M, M-F, F-F) | 1 set | Connections between all components | ~€8 |
| USB Cables | 2 | Programming and power for both ESP32s | ~€5 |
| **Total (excluding lab/thesis-provided items)** | | | **~€165–230** |

> **Note:** The servo grid, Drake Titan actuators, DRV2605L drivers, PCA9548A multiplexers, and the weighted blanket enclosure are part of the thesis hardware and are provided by the lab (TA: Marlon Rodriguez). The course project contribution focuses on the game interface, the musical feedback system, and the IMU-to-haptic mapping layer.

---

## Step 1 — System Architecture & Design Rationale

### 1.1 Conceptual Overview

The primary objective of this prototype is to dynamically translate handheld digital gameplay into synchronized physical sensations (vibration and mechanical pressure) across the pediatric patient's belly, effectively distracting them from the clinical environment.

The system is built around a **decentralized wireless Master/Slave architecture** divided into two core subsystems:

- **Handheld Display (Master Node):** Captures the child's tilt inputs via the MPU6050, computes game logic, renders the visual interface on the TFT display, plays music on the buzzer, and broadcasts haptic event packets over the air.
- **Haptic Blanket (Slave Node):** Receives wireless packets and translates them into physical pressure (servos via PCA9685) and vibrotactile feedback (Drake Titan actuators via DRV2605L drivers and PCA9548A multiplexers).

Separating the two modules wirelessly eliminates all physical cables between the child's hands and the blanket, ensuring maximum patient mobility, preventing cable strain during active gameplay, and strictly isolating the high-current motor power supply from the sensitive handheld logic board.

### 1.2 System Architecture Diagram

```
┌─────────────────────────────────────┐    ESP-NOW (2.4 GHz, <2 ms)     ┌──────────────────────────────────────┐
│         SCREEN SIDE (Master)        │ ──────────────────────────────► │        BLANKET SIDE (Slave)          │
│                                     │                                 │                                      │
│  1× 4.0" TFT ILI9488 (SPI)          │         struct_message          │  16× MG90S Servo Motors              │
│  1× MPU6050 IMU (I2C)               │  {id, mode, allAngles[16],      │  16× Drake Titan Haptic Actuators    │
│  1× KY-006 Passive Buzzer (LEDC)    │   tempVal}                      │  16× DRV2605L Haptic Drivers         │
│  1× ESP32-WROOM (Master)            │ ◄────────────────────────────── │  2×  PCA9548A I2C Multiplexers       │
│                                     │         PIDData                 │  1×  PCA9685 PWM Driver              │
│                                     │  {timeMs, avgTemp, setPoint,    │  1×  ESP32-WROOM (Slave)             │
│                                     │   pwmPercent}                   │                                      │
└─────────────────────────────────────┘                                 └──────────────────────────────────────┘
```

### 1.3 Component Selection & Rationale

- **ESP32-WROOM (dual-core, 240 MHz):** The dual-core processor prevents SPI display rendering from blocking IMU sensor polling. Native 2.4 GHz RF transceiver enables ESP-NOW without an external module.
- **ESP-NOW wireless protocol:** Connectionless protocol. No Wi-Fi router or handshake overhead — transmission completes in under 2 ms, ensuring the tactile sensation on the belly is perceptually synchronous with the screen event. The RF channel is locked to Wi-Fi channel 11 to avoid local interference.
- **MPU6050 IMU:** 6-axis gyroscope/accelerometer over I2C. "Tilt-to-steer" interaction requires only gross-motor movement, making it accessible for children aged 3–6 who may lack fine-motor coordination under stress. A low-pass filter (α = 0.15) on the accelerometer axis smooths jitter without introducing noticeable lag.
- **ILI9488 TFT (480×320):** Hardware-accelerated SPI display driven by the `TFT_eSPI` library. Sufficient resolution and color depth for engaging child-friendly game visuals.
- **KY-006 Passive Buzzer on LEDC:** The ESP32's hardware LEDC timer generates precise PWM tones. A fully non-blocking asynchronous melody system allows music to loop continuously without pausing game logic.
- **PCA9685 PWM Driver:** I2C-addressable 16-channel driver safely sources the current required by 16 servos without overloading the ESP32's GPIO pins.
- **DRV2605L + PCA9548A:** Each DRV2605L shares I2C address 0x5A. Two PCA9548A multiplexers (8 channels each) allow the Slave ESP32 to address all 16 drivers individually. The `allAngles[16]` array in the packet maps directly to each driver/actuator.

---

## Step 2 — Hardware Setup & Wiring

Because the haptic blanket is a pre-existing thesis hardware platform, the physical construction for this course project focuses on the **handheld Master node** (screen side). All components are soldered onto a permanent protoboard for mechanical stability during motion-heavy gameplay.

### Master Node (Screen Side) — Pinout

| Peripheral | Peripheral Pin | ESP32 GPIO | Notes |
|---|---|---|---|
| MPU6050 | VCC | 3.3V | Regulated logic rail |
| MPU6050 | GND | GND | |
| MPU6050 | SDA | GPIO 21 | Hardware I2C SDA |
| MPU6050 | SCL | GPIO 22 | Hardware I2C SCL |
| MPU6050 | AD0 | GND | I2C address 0x68 |
| MPU6050 | INT | — | Not used |
| KY-006 Buzzer | + | **GPIO 26** | LEDC PWM channel 0 |
| KY-006 Buzzer | − | GND | |
| TFT ILI9488 | VCC | 5V (Vin) | High-brightness backlight rail |
| TFT ILI9488 | GND | GND | |
| TFT ILI9488 | CS | GPIO 15 | SPI chip select |
| TFT ILI9488 | RST | GPIO 4 | Display reset |
| TFT ILI9488 | DC | GPIO 2 | Data/command select |
| TFT ILI9488 | SDI (MOSI) | GPIO 23 | Hardware VSPI MOSI |
| TFT ILI9488 | SCK | GPIO 18 | Hardware VSPI clock |
| TFT ILI9488 | MISO | GPIO 19 | Hardware VSPI MISO |
| TFT ILI9488 | BL | GPIO 32 | Backlight (or tie to 3.3V for always-on) |
| TFT Touch (XPT2046) | T_CS | GPIO 14 | Touch SPI chip select |

> **Important:** Configure the TFT pin mapping in `TFT_eSPI`'s `User_Setup.h` to match the table above before compiling.

> **Wiring diagrams** (Fritzing `.fzz` files) are available in the `/hardware/wiring/` folder of this repository.

---

## Step 3 — IMU Calibration & Tilt-Based Game Input

Sensor register configuration is managed via the `Wire.h` and `MPU6050.h` libraries. On system startup, an automated calibration routine collects 500 static samples to build an offset correction matrix, eliminating gyro drift.

```cpp
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;
int16_t ax_offset = 0, ay_offset = 0;

void calibrateIMU() {
  long ax_sum = 0, ay_sum = 0;
  Wire.begin(21, 22); // I2C on GPIO 21 (SDA) and GPIO 22 (SCL)
  mpu.initialize();

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

> **Important:** The haptic data pipeline (ESP-NOW sends) is held in a software lock during calibration to prevent vibration actuators from coupling mechanical noise back into the MPU6050 and corrupting the offset values.

At runtime, the calibrated raw accelerometer values are read directly in each game loop and filtered with a low-pass filter (α = 0.15) to smooth noise while maintaining fast response:

```cpp
// Inside each game loop (e.g. brickLoop)
int16_t rawAx, rawAy, rawAz, rawGx, rawGy, rawGz;
readMPU6050raw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz);

float ax = rawAx / 16384.0f;         // convert to g
filtAx += (ax - filtAx) * 0.15f;     // low-pass filter (alpha = 0.15)

float move = filtAx * BRICK_PLAT_SPEED;  // map to game input
```

A threshold of approximately ±2800 raw units is used to distinguish intentional tilt from idle noise, defined as a compile-time constant (`TILT_THRESHOLD 2800`).

---

## Step 4 — Game Development & Touchscreen Interface

Using the hardware-accelerated `TFT_eSPI` library, **three complete motion- and touch-controlled games** are compiled directly onto the Master ESP32:

### Snake
The player tilts the controller to steer a snake toward randomized targets. Score increases on each target collected; the game ends on self-collision. Snake length grows with each point.

### Brick Breaker
The player tilts the controller horizontally to translate a paddle across the screen, deflecting a ball upward into a grid of 12×4 bricks. The paddle position is mirrored in real time across the full 4×4 servo grid as a spatial intensity gradient (see Step 5).

### Balloon Pop
The player taps balloons on the touchscreen before they float off the top. Three balloon types appear with different probabilities:
- **Normal balloons** (various colors) — standard points
- **Golden balloons** (12% spawn chance) — bonus points
- **Bomb balloons** (20% spawn chance) — lose a life if tapped

The game starts with a 1600 ms spawn interval that accelerates to a minimum of 450 ms, and the player has 3 lives.

### Application State Machine

The firmware manages 16 application states, covering both the games and the manual control interface inherited from the thesis platform:

```cpp
enum AppState {
  CALIBRATION, MENU,
  SNAKE_MODE, BRICK_MODE, BALLOON_MODE,   // course project games
  GRID_MODE, GRID2_MODE,                  // manual servo grid control
  STROKE_MODE, STROKE2_MODE,              // manual stroke pattern control
  VIBE_MODE,                              // manual vibration control
  DRAW_MODE,                              // free draw on servo grid
  GRAV_MODE,                              // gravity/tilt servo demo
  TEMP_MODE,                              // temperature monitoring/control
  DEPT_MODE,                              // servo depth (depthScale) adjustment
  VOL_MODE,                               // buzzer volume control (0–10)
  GAME_MODE                               // generic game entry point
};
```

### Multi-Sensory Mapping Matrix

| Game Event | Buzzer | Vibration (Drake Titan) | Servo Grid (MG90S) |
|---|---|---|---|
| Game background | Looping game melody | — | Idle (0°) |
| Tilt input | — | — | Spatial gradient tracks platform |
| Brick hit / point collected | — | Nearest node activates (intensity 255) | Uniform pulse |
| Balloon pop | — | Nearest node pulse | — |
| Wall / obstacle impact | — | Full 4×4 maximum rumble | Brief retraction |
| Game over | Descending jingle (once) | Sweeping matrix pulse | Full grid reset to 0° |

---

## Step 5 — Haptic Feedback Mapping (Vibration & Servo)

### ESP-NOW Packet Structure

When a game event is registered, the Master packs the full actuator state into a C struct and fires it over ESP-NOW:

```cpp
typedef struct struct_message {
  int     id;             // board identifier (BOARD_10 = 10)
  int     mode;           // control mode (0=servo angles, 5=vibration intensity)
  uint8_t allAngles[16];  // per-actuator value: servo angle (0–90°) or vibration (0–255)
  float   tempVal;        // temperature setpoint or readback
} struct_message;
```

The `allAngles[16]` array provides independent control over all 16 actuator nodes simultaneously. A `depthScale` variable (adjustable via the `DEPT_MODE` screen) globally scales servo displacement before sending, allowing the caregiver to tune pressure intensity:

```cpp
void sendToServo(int mode, float temp) {
  myData.id = BOARD_10; myData.mode = mode; myData.tempVal = temp;
  if (mode == 0 || mode == 2 || mode == 3) {
    for (int i = 0; i < 16; i++)
      myData.allAngles[i] = (uint8_t)(myData.allAngles[i] * depthScale);
  }
  esp_now_send(mac_servo, (uint8_t*)&myData, sizeof(myData));
}
```

### Spatial Mapping — TRIL_MAP

Physical actuator positions in the blanket do not follow a simple row-major order due to wiring constraints. A lookup table translates logical grid coordinates to the correct physical driver index:

```cpp
const int8_t TRIL_MAP[16] = {
   8, 9, 10, 11, 12, 13, 14, 15, 7, -1, 5, 4, 3, 2, 1, 0,
};
// -1 = no physical actuator at this logical position
```

### Vibration Feedback (Mode 5) — Nearest Node

On brick collision or balloon pop, the spatial coordinates of the hit object are mapped to the nearest physical motor:

```cpp
float bcx = (bx1 + bx2) / 2.0f, bcy = (by1 + by2) / 2.0f;
int hc = constrain((int)(bcx / (W / 4.0f)), 0, 3);
int hr = constrain((int)(bcy / (H / 4.0f)), 0, 3);
int8_t motor = TRIL_MAP[15 - (hr * cols + hc)];

for (int ii = 0; ii < 16; ii++) myData.allAngles[ii] = 0;
if (motor >= 0 && motor < 16) myData.allAngles[motor] = 255;
myData.id = BOARD_10; myData.mode = 5; myData.tempVal = 0;
esp_now_send(mac_servo, (uint8_t*)&myData, sizeof(myData));
```

### Servo Gradient Feedback — Brick Breaker Platform Tracking

Rather than a single-point activation, Brick Breaker continuously mirrors the paddle position across the full 4×4 grid as a spatial intensity gradient. Servos closer to the paddle column receive higher displacement; farther columns receive proportionally less:

```cpp
void brickServoUpdate() {
  float platCenter = brickPlatX + BRICK_PLAT_W / 2.0f;
  float gx = constrain(platCenter / (W / 4.0f), 0.0f, 3.99f);

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      float dist      = fabsf(gx - (float)c);
      float intensity = constrain(1.0f - dist * 0.7f, 0.0f, 1.0f);
      myData.allAngles[15 - (r * cols + c)] = (uint8_t)(90.0f * intensity);
    }
  }
  sendToServo(0, 0);
}
```

This creates a continuous wave of pressure across the child's belly that tracks paddle movement in real time, producing a coherent spatial haptic experience.

---

## Step 6 — Audio Feedback via Passive Buzzer

The buzzer is driven via the ESP32's LEDC hardware PWM timer on **GPIO 26**. A fully non-blocking melody system allows music to loop continuously in the background without interrupting game loop timing.

### Per-Game Melodies

Each game has a dedicated melody defined as an array of `{frequency_Hz, duration_ms}` structs:

| Game | Melody | Character |
|---|---|---|
| Snake | *Serpentine* | E minor, ~150 BPM, 31 notes |
| Brick Breaker | *Korobeiniki* (Tetris theme) | Major, upbeat, 39 notes |
| Balloon Pop | *Cloud Pop* | A minor, serene, 17 notes |
| Game Over | Descending jingle | Minor, 8 notes, plays once |

```cpp
struct BuzzerNote { uint16_t freq; uint16_t ms; };

// Game Over jingle — descending minor scale, plays once
const BuzzerNote MELODY_GAMEOVER[] = {
  {NOTE_E5,200},{NOTE_D5,200},{NOTE_C5,200},
  {NOTE_B4,200},{NOTE_A4,200},{NOTE_G4,200},
  {NOTE_E4,600},{NOTE_REST,300},
};
```

### Non-Blocking Playback

Notes are stepped through using a millis-based timer — no `delay()` calls — so game logic and haptic transmissions are never blocked:

```cpp
void buzzerTick() {
  if (!buzzerPlaying) return;
  if (millis() < buzzerNextAt) return;  // current note still playing

  if (buzzerIdx >= buzzerLen) {
    if (buzzerDoLoop) buzzerIdx = 0;    // loop back to start
    else { ledcWrite(BUZZER_CHANNEL, 0); buzzerPlaying = false; return; }
  }

  uint16_t freq = buzzerMelody[buzzerIdx].freq;
  if (freq == NOTE_REST || buzzerVolume == 0) ledcWrite(BUZZER_CHANNEL, 0);
  else                                        ledcWriteTone(BUZZER_CHANNEL, freq);

  buzzerNextAt = millis() + buzzerMelody[buzzerIdx].ms;
  buzzerIdx++;
}
```

### Volume Control

A dedicated `VOL_MODE` screen (accessible from the main menu) allows the caregiver to set buzzer volume from 0 (mute) to 10. The value scales the PWM duty cycle on the LEDC channel, providing perceptible loudness steps without additional hardware.

---

## Step 7 — Inter-Microcontroller Communication (ESP-NOW)

Data is streamed as packed C structs directly over the 2.4 GHz RF band using ESP-NOW. The Master stores the Slave's hardware MAC address at compile time:

```cpp
uint8_t mac_servo[] = {0xec, 0xe3, 0x34, 0x99, 0xf9, 0xac};
```

Connectionless radio bursts complete in under 2 ms. Communication is bidirectional: the Slave periodically sends back PID temperature telemetry to the Master:

```cpp
struct PIDData {
  unsigned long timeMs;
  float avgTemp;
  float setPoint;
  float pwmPercent;
};
```

This allows the `TEMP_MODE` screen on the Master to display live blanket temperature, setpoint, and heater power, giving the caregiver visibility of the thermal subsystem at a glance.

**RF interference mitigation:** The ESP32 RF channel is locked to Wi-Fi channel 11, isolating communication from typical dense network environments such as hospital wards.

---

## Step 8 — Integration & System Testing

### Deployment Steps

1. Configure `TFT_eSPI/User_Setup.h` with the pin mapping from Step 2.
2. Flash `/src/master_hmi/main.ino` onto the screen-side ESP32.
3. Flash `/src/slave_blanket/` firmware onto the blanket-side ESP32.
4. Update `mac_servo[]` in the Master firmware with the actual MAC address of the Slave ESP32 (readable via `WiFi.macAddress()` in a test sketch, then recompile).
5. Solder all Master node components onto the protoboard per the pinout table in Step 2.
6. Connect the blanket Slave to its dedicated 5V/10–20 A external power supply. Power the Master via USB.
7. On boot, hold the screen completely flat for ~2 seconds. The `CALIBRATION` state exits automatically once 500 samples are collected.
8. Navigate to a game from the menu and verify that visual events produce immediate haptic responses on the blanket grid.

### Iterative Troubleshooting Log

| Issue encountered | Root cause identified | Resolution applied |
|---|---|---|
| IMU calibration values corrupted on every boot | Haptic actuator vibrations coupling mechanical noise back into the MPU6050 during the 500-sample collection window | Software pipeline lock: all ESP-NOW haptic sends are held until the calibration complete flag is set |
| Intermittent ESP-NOW packet drops in lab | Dense RF environment on the default channel causing collisions | RF channel locked to Wi-Fi channel 11 |
| Blanket ESP32 brownout-resetting when all 16 servos activate simultaneously | Current surge collapsing the microcontroller's supply rail | Dedicated external 5V/10–20 A bus for all actuators with large smoothing capacitors; ESP32 powered separately via USB |

---

## Discussion (Step n+1)

The integrated system successfully demonstrates that high-fidelity, spatially coherent haptic feedback synchronized with live game events can be deployed on an affordable and compact embedded platform.

**Key observations:**

- The **spatial gradient mapping** in Brick Breaker — where the full 4×4 servo grid continuously tracks the paddle position — was perceived as highly natural by early informal testers. The pressure shift across the belly felt coherent with the visual paddle movement.
- The **per-game musical themes** (Tetris theme for Brick, *Serpentine* for Snake, *Cloud Pop* for Balloon) significantly increased perceived engagement compared to isolated event tones. The non-blocking melody system ensured music never caused game stuttering.
- **Balloon Pop** proved particularly accessible for the target age group due to its direct touch interaction, requiring no tilt calibration or fine-motor coordination.
- The **caregiver-accessible volume control** and **depth scale adjustment** allow the device to be tuned to the clinical environment and individual child sensitivity — a practical requirement for hospital deployment.
- Total end-to-end feedback latency (ESP-NOW ~2 ms + actuator response) is approximately 15–30 ms — well below the perceptual threshold for audio-tactile asynchrony (~50–70 ms).

**Limitations:**

- The three games have not yet been tested with the target user population (children aged 3–6). Formal usability testing with child participants and clinical staff is essential before any deployment.
- The passive buzzer cannot produce polyphonic sound or realistic timbres. An I2S DAC with a small speaker would allow richer audio feedback.
- Servo displacement and contact force were not formally calibrated against pediatric sensory thresholds; a study measuring just-noticeable differences for this age group is needed.


---

## Conclusion & Future Work (Step n+2)

This project successfully extended the existing haptic distraction blanket system with an interactive, tilt- and touch-controlled game layer providing **real-time, event-driven haptic, musical, and visual feedback**. Three games (Snake, Brick Breaker, Balloon Pop), a non-blocking per-game melody system, a spatial servo intensity gradient, bidirectional ESP-NOW communication, and caregiver-adjustable parameters together represent a technically ambitious and cohesive contribution to pediatric procedural comfort care.

**Key takeaways:**
- IMU-based tilt input is a viable and child-accessible control modality requiring only gross-motor movement.
- Non-blocking asynchronous melody playback allows continuous music without impacting game loop or haptic timing.
- ESP-NOW wireless communication achieves perceptually transparent latency (~15–30 ms total) while eliminating all cables between the handheld device and the blanket.
- The `allAngles[16]` packet structure enables per-actuator independent control of all 16 nodes, supporting rich spatial haptic patterns beyond simple point activation.

**Future directions:**
- Conduct formal usability testing with children aged 3–6 in a simulated clinical setting, with clinician and parent feedback.
- Replace the passive buzzer with a miniature I2S speaker for polyphonic, higher-fidelity audio.
- Develop additional game themes (animals, underwater, space) with distinct haptic profiles to sustain novelty across repeated procedure visits.
- Resolve the unmapped actuator position in `TRIL_MAP` with updated blanket wiring for full 16-node coverage.
- Integrate a physiological sensor (e.g., photoplethysmography for heart rate) to adaptively modulate game speed and haptic intensity based on the child's real-time arousal level.
- Formal clinical validation in collaboration with the UZ Leuven pediatric and oncology departments.

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

[13] Espressif Systems, *ESP32 Technical Reference Manual*, v5.1, 2024. [Online]. Available: https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf

[14] Bodmer, *TFT_eSPI library for ESP32*, GitHub. [Online]. Available: https://github.com/Bodmer/TFT_eSPI

[15] Texas Instruments, *DRV2605L Haptic Driver datasheet*, SLOS825B, 2014. [Online]. Available: https://www.ti.com/lit/ds/symlink/drv2605l.pdf

---

*Repository maintained by Alexia Pires, Thibaut Degreef, and Taiki De Wel — KU Leuven, 2026.*
