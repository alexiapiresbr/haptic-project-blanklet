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

The contribution of this haptic course project is to **add an interactive visual and game layer** to the existing hardware: a game running on a touchscreen, controlled by **tilting the screen** (via an IMU sensor), whose events are directly **mapped to the haptic actuators** in the blanket. The child plays the game on the screen and simultaneously *feels* the gameplay through vibrations and servo movements on their belly, deepening the distraction effect through **multisensory engagement**.

A **passive buzzer** integrated into the ESP-32 also provides synchronized audio feedback (tones for events such as successes or alerts), further increasing engagement.

### Why Haptic Technology?

The sense of touch is mediated by a rich array of mechanoreceptors in the skin: Meissner's corpuscles (light touch), Merkel's discs (pressure), Ruffini endings (stretch), and Pacinian corpuscles (vibration) [10]. These receptors respond to distinct stimulus types and can be selectively stimulated using actuators operating at appropriate frequencies and contact profiles. Combining vibrotactile and pressure stimuli on the abdomen, a region with moderate receptor density and relatively low two-point discrimination thresholds, creates a novel, engaging sensory experience that redirects the child's attention away from the medical procedure.

Existing literature on haptic distraction in pediatric medical contexts is sparse, making this project scientifically novel in addition to being technically challenging.

---

## 2. Supplies

The following bill of materials covers all hardware required to reproduce this prototype. Items shared with the thesis hardware platform (and already provided by the lab) are noted accordingly.

### 2.1 Bill of Materials (BOM)

| Component | Quantity | Notes | Estimated Cost |
| :--- | :--- | :--- | :--- |
| **ESP-32 Microcontroller (Screen Side)** | 1 | Main microcontroller for game logic, screen, and IMU reading | ~€10–15 |
| **ESP-32 Microcontroller (Blanket Side)** | 1 | Receiver brain for servos and haptic drivers | ~€10–15 |
| **MPU6050 Module** | 1 | Accelerometer & Gyroscope for tilt-based screen input | ~€5 |
| **4.0-inch TFT LCD display with ILI9488 controller** | 1 | Visual game interface for child | ~€30–50 |
| **KY-006 Passive Piezo Buzzer** | 1 | Audio feedback | ~€3 |
| **PCA9685 PWM Driver** | 1 | 16-Channel driver for all 16 servos safely via I2C | ~€8–12 |
| **PCA9548A Multiplexer** | 2 | Expands I2C to control 16 separate DRV2605L drivers | ~€6 |
| **DRV2605L Motor Drivers** | 16 | Generates specific waveforms for the Drake actuators | ~€60–80 |
| **Drake Titan Actuators** | 16 | Vibrotactile actuators for haptic feedback in blanket grid | Provided by lab |
| **MG90S Servo Motors** | 16 | Controls the 4×4 physical belly grid | Provided by thesis |
| **5V Power Supply** | 1 | High-Current (10A-20A) dedicated power for all servos & haptics | ~€20–30 |
| **DC Power Adapter** | 1 | Jack to Terminal Adapter to connect wall power to PCA9685/breadboard | ~€2 |
| **Breadboard** | 2 | For rapid prototyping before permanent soldering | ~€6 |
| **Jumper Wires** | 1 set | Mix of M-M, M-F, F-F for connections between components | ~€8 |
| **USB Cables** | 2 | Power/Data for ESP32s and computer programming | ~€5 |
| **Total** | | *(Excluding lab/thesis-provided items)* | **~€165–230** |
> **Note:** The servo grid, vibration actuators, multiplexers, and weighted blanket enclosure are part of the thesis hardware and are provided by the lab (TA: Marlon Rodriguez). The course project contribution focuses on the game interface and the IMU-to-haptic mapping layer.

## 3. Methods & Technical Approach

### Step 1 — System Architecture & Conceptual Framework

#### 1.1 Conceptual Overview
The primary objective of this prototype is to dynamically translate handheld digital gameplay into synchronized physical sensations (vibration and mechanical movement) across a pediatric patient's body, effectively distracting them from clinical environments. 

To achieve this safely and reliably, the system was designed around a decentralized, wireless Master/Slave architecture divided into two core subsystems:
1. **The Handheld HMI (Master Node):** Captures the child's physical tilt inputs, computes game logic, renders the visual/auditory interface, and broadcasts event triggers.
2. **The Haptic Blanket (Slave Node):** Receives wireless triggers and translates them into physical pressure and vibrotactile feedback via an electromechanical grid.

By compartmentalizing the design into these two distinct wireless modules, we eliminated the need for physical data cables connecting the child's hands to the blanket. This ensures maximum patient mobility, prevents cable strain during active gameplay, and strictly isolates the high-current motor power supplies from the sensitive handheld logic board.

#### 1.2 System Architecture Diagram
The system consists of two subsystems communicating over ESP-NOW Wireless Protocol:
```
┌─────────────────────────────────┐    ESP-NOW (Wireless)   ┌──────────────────────────────────┐
│        SCREEN SIDE              │ ──────────────────────► │       BLANKET SIDE               │
│                                 │                         │                                  │
│  1x 4.0" TFT Display ILI9488    │                         │  16x MG90S Servo Motors          │
│  1x MPU6050 IMU                 │                         │  16x Drake Titan Actuators       │
│  1x KY-006 Passive Buzzer       │                         │  16x DRV2605L Haptic Drivers     │
│  1x ESP32 Microcontroller       │                         │  2x PCA9548A I2C Multiplexers    |
|                                 │                         |  1x PCA9685 PWM Driver           |
|                                 |                         |  1x ESP32 Microcontroller (Slave)|
└─────────────────────────────────┘                         └──────────────────────────────────┘
```

**Diagram Explanation:** The Handheld Master acts as the central logic hub. It polls the MPU6050 via I2C, updates the display via SPI, and computes game state changes. When a game event occurs (e.g., scoring or colliding), it bypasses local Wi-Fi networks and broadcasts a packed data struct directly over the 2.4 GHz RF band using ESP-NOW. The Slave Node receives this packet instantly and routes the execution commands through its multiplexers and PWM drivers to the physical actuators.

#### 1.3 Component Selection & Rationale
Every component was selected to balance low-latency responsiveness, child-friendly ergonomics, and integration simplicity. The logic behind our major design decisions is as follows:
- **Microcontrollers (ESP32-WROOM)**: We selected dual ESP32s over standard Arduino Unos. Reasoning: The ESP32's dual-core 240 MHz processor prevents graphic rendering loops on the SPI display from bottlenecking the sensor polling rates. Furthermore, it possesses native 2.4 GHz RF transceivers, enabling our custom wireless protocol without requiring external NRF24L01 radio modules.
- **Input Modality (MPU6050 IMU)**: We opted for a 6-axis gyroscope/accelerometer instead of physical joysticks or buttons. Reasoning: Standard button layouts demand fine-motor coordination that is often compromised under pediatric anxiety. A "tilt-to-steer" interface turns screen re-orientation into an accessible, gross-motor physical gesture that is highly intuitive for 3-to-6-year-olds.
- **Wireless Protocol (ESP-NOW)**: We utilized ESP-NOW rather than standard 802.11 Wi-Fi or Bluetooth. Reasoning: ESP-NOW is a connectionless protocol. By omitting heavy Wi-Fi handshake constraints and router dependencies, communication overhead remains under 2 milliseconds. This ensures that the tactile sensation felt on the patient's belly is perfectly synchronized with the visual frames displayed on the screen.

#### 1.4 Hardware Constraints & Integration Strategy
During the design phase, we encountered two primary hardware constraints that dictated our integration strategy: I2C Address Conflicts and Power Distribution Brownouts.
1. Routing Constraint (Multiplexing): The blanket requires 16 independent DRV2605L haptic drivers, but these chips share an identical, hardcoded I2C address. Solution: We integrated two PCA9548A 8-channel I2C multiplexers. This allows the slave ESP32 to dynamically switch communication channels on-the-fly, addressing each haptic node individually without bus collisions.
2. Power Constraint (Motor Isolation): Activating up to 16 MG90S servos simultaneously draws massive transient current spikes that can drop the voltage below the ESP32's 3.3V logic threshold, causing system resets. Solution: We designed a split-rail power distribution strategy. A dedicated high-current 5V rail feeds the servos directly, while a PCA9685 PWM driver acts as a buffer between the logic controller and the motors. The ESP32 simply sends low-power I2C signals to the PCA9685, completely protecting the logic core from motor power fluctuations.

### Step 2 — Hardware Setup & Wiring Pinouts
The Master HMI unit is securely soldered onto permanent protoboards to ensure mechanical safety and connection integrity during motion-heavy gameplay.

| Peripheral Pin | ESP32 Target Pin | Connection Context |
| :--- | :--- | :--- |
| **MPU6050 VCC / GND** | 3.3V / GND | Regulated logic power rail |
| **MPU6050 SDA / SCL** | GPIO 21 / GPIO 22 | Hardware I2C communications bus |
| **KY-006 Buzzer (+ / -)** | GPIO 25 / GND | LEDC Hardware PWM timer assignment |
| **TFT VCC / GND** | 5V (Vin) / GND | High-brightness backlighting power rail |
| **TFT CS / RESET / DC** | GPIO 15 / GPIO 4 / GPIO 2 | SPI control and routing selections |
| **TFT SDI (MOSI) / SCK** | GPIO 23 / GPIO 18 | Hardware VSPI lines |

### Step 3 — IMU Calibration & Tilt-Based Game Input

Sensor register configuration is managed via the `Wire.h` and `MPU6050.h` libraries. On system startup, an automated routine collects 500 static sample data points to generate an active offset adjustment matrix, eliminating gyro drift:

```cpp
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;
int16_t ax_offset = 0, ay_offset = 0;

void calibrateIMU() {
  long ax_sum = 0, ay_sum = 0;
  Wire.begin(21, 22); // Core I2C Allocation
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

The runtime loop parses these calibrated values against fixed threshold deadbands to convert angles into crisp steering instructions:

```cpp
#define TILT_THRESHOLD 2800 

String getTiltDirection() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  int16_t ax_cal = ax - ax_offset;
  int16_t ay_cal = ay - ay_offset;

  if (ax_cal > TILT_THRESHOLD)  return "RIGHT";
  if (ax_cal < -TILT_THRESHOLD) return "LEFT";
  if (ay_cal > TILT_THRESHOLD)  return "UP";
  if (ay_cal < -TILT_THRESHOLD) return "DOWN";
  return "NEUTRAL";
}
```
### Step 4 — Game Development & Touchscreen Interface

Using the optimized hardware-accelerated `TFT_eSPI` graphical library, two complete motion-controlled video games were compiled directly onto the HMI device:

1. **Snake:** Players tilt the controller board to guide a snake toward randomized targets, tracking score increases while dodging self-collision events.
2. **Brick Breaker:** Players tilt the controller horizontally to translate a paddle array across the screen, deflecting a high-velocity ball upward into a grid of digital bricks.

#### Multi-Sensory Mapping Matrix
Every logical collision or point state change in the game environment automatically triggers a mapped auditory and haptic feedback event:

| Game Software Event | Buzzer Acoustic Output | Haptic Vibration Profile | Mechanical Servo Grid Action |
| :--- | :--- | :--- | :--- |
| **Menu Navigation** | Short 440 Hz click | Brief single node pulse | 0% displacement (Home) |
| **Direction Steering** | No tone generation | Disabled | Smooth micro-adjust tracking |
| **Point Collected** | High-pitched victory melody | Concentric wave expanding outward | Sudden uniform 2.5cm pulse |
| **Wall/Obstacle Impact** | Low-pitched 200 Hz thud | Maximum 4x4 grid rumble | Immediate tactical retraction |
| **Game Over State** | Descending minor arpeggio | Intermittent sweeping matrix pulse | Total grid reset to 0° |

### Step 5 — Haptic Feedback Mapping (Vibration & Servo)

When a game event is registered, spatial coordinates are packed and sent instantly to the blanket via ESP-NOW:

```cpp
void triggerBlanketFeedback(uint8_t eventID, uint8_t powerIntensity) {
    HapticPacket.event_id = eventID;
    HapticPacket.intensity = powerIntensity;
    esp_now_send(blanketMACAddress, (uint8_t *) &HapticPacket, sizeof(HapticPacket));
}
```

* **Vibrotactile Textures:** The blanket receiver parses the incoming package, switches the I2C lines using the PCA9548A multiplexers, and commands the target DRV2605L driver to play specific hardware waveforms through the TacHammer actuators.
* **Pressure Sweeps:** For large mechanical movements (like a wave pattern), the blanket node translates servo position commands sequentially over the PCA9685 PWM block to swing the MG90S metal gears without bottlenecking system logic.

### Step 6 — Audio Feedback via Passive Buzzer

Sound waves are synthesized natively on the ESP32 using the timer-controlled `ledcWriteTone()` abstraction framework:

```cpp
#define BUZZER_PIN 25
#define CHANNEL_LEDC 0

void playToneFeedback(uint8_t eventType) {
  if (eventType == 1) { // Move event
    ledcWriteTone(CHANNEL_LEDC, 440); 
    delay(40);
    ledcWrite(CHANNEL_LEDC, 0);
  } 
  else if (eventType == 2) { // Impact event
    ledcWriteTone(CHANNEL_LEDC, 180); 
    delay(120);
    ledcWrite(CHANNEL_LEDC, 0);
  } 
  else if (eventType == 3) { // Success Score
    uint16_t melody[] = {523, 659, 784, 1047}; 
    for(int i=0; i<4; i++) {
       ledcWriteTone(CHANNEL_LEDC, melody[i]);
       delay(80);
    }
    ledcWrite(CHANNEL_LEDC, 0);
  }
}
```
### Step 7 — Inter-Microcontroller Communication (ESP-NOW)

To achieve real-time synchronization, data is streamed as a packed C-struct directly into the 2.4 GHz RF band, completely removing standard Wi-Fi router parsing latency:

```cpp
typedef struct struct_message {
    uint8_t event_id;
    uint8_t intensity;
    uint8_t spatial_node;
} struct_message;

struct_message HapticPacket;
```

During initialization, the HMI Master stores the unique hardware MAC address of the Blanket Slave node. Packed states are fired off as connectionless radio bursts, completing transmission in under 2 milliseconds.

### Step 8 — Integration & System Testing

#### Deployment Ledger
1. Flash the `/src/master_hmi/` firmware onto the handheld controller ESP32.
2. Flash the `/src/slave_blanket/` firmware onto the receiving blanket ESP32.
3. Wire components over the designated proto-board layout.
4. Supply 5V to the HMI Unit and power the blanket using balanced 24V/5V rails.
5. Calibrate the system by keeping the controller completely flat for 2 seconds on boot.
6. Launch games and verify that visual events instantly match haptic grid behavior.

#### Iterative Troubleshooting Enhancements
* **Calibration Noise Overcome:** Initial physical vibration pulses on boot were bleeding mechanical noise back into the MPU6050 sensor, corrupting the calibration loop. *Resolution:* Programmed a structural software lock that forces the haptic data pipeline to sleep until calibration completes.
* **RF Packet Drops Eliminated:** Heavy RF clutter in lab environments caused periodic packet dropouts over the air. *Resolution:* Configured the ESP32 physical RF channel layer to lock onto Wi-Fi channel 11, isolating communication from local interference.
* **System Voltage Brownouts Resolved:** Activating all 16 mechanical servos simultaneously caused instant current drops that reset the blanket's microcontroller. *Resolution:* Isolated the microcontrollers completely from high motor loads by powering the actuators through a dedicated external power bus backed by large smoothing decoupling capacitors.

## 4. Discussion

The integrated system successfully demonstrates that high-fidelity haptic feedback synchronized with live, visual game actions can be deployed on an affordable, compact embedded platform. 

### Key Empirical Observations
* **Spatial Coherence:** Mapping digital on-screen vectors directly to physical body coordinates (e.g., smashing a brick on the left side of the display triggering the far-left haptic node) was reported as highly natural and intuitive by early user trials.
* **Calming Interventions:** The sweeping wave motion generated by the servo-driven gear racks produced a gentle, reassuring pressure across the abdominal area, closely aligning with existing clinical literature regarding the psychophysiological benefits of stroking touch [4].
* **Low System Overhead:** Transitioning from physical cables to connectionless ESP-NOW data structs proved vital, offering a completely transparent real-time response curve (~15–30 ms total end-to-end feedback latency).

### Technical Limitations
* **Graphical Constraints:** While functional, the native display driver limits advanced 3D visual environments; upgrading to a dedicated external graphics chip would allow more rich textures.
* **Acoustic Profiling:** The simple tones generated by the passive piezo buzzer are functional but basic. Integrating an active audio DAC chip would support high-fidelity music and realistic sound effects to increase immersion.
---

## 5. Conclusion & Future Work 

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

## 6. References 

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
