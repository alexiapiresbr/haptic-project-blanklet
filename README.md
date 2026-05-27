# Haptic Technology to Distract Small Children During Medical Procedures

**Course:** B-KUL-T4lMD2 Haptic Interfaces Experience — KU Leuven, Department of Mechanical Engineering  
**Authors:** Alexia Pires, Taiki De Wel, Thibaut Degreef  
**Supervisor:** Prof. Dr. ir. Carlos Rodriguez-Guerrero  
**Teaching Assistants:** Marlon Rodriguez, Ewald Ury  
**Academic Year:** 2025–2026

---

## 1. Introduction

Pediatric anxiety during medical procedures is one of the most consistently documented stressors in paediatric care. Children undergoing hospital treatments face an environment that is inherently threatening: unfamiliar surroundings, loss of bodily autonomy, and repeated encounters with medical personnel [1]. For children aged 3 to 6 this distress is particularly pronounced they lack the verbal tools to process fear and instead express it through crying, clinging, or active resistance [3]. Even minimally invasive interventions such as venipuncture or bandage removal provoke significant acute distress [4], and repeated procedures over a long treatment course can psychologically traumatise the child [5].

Within clinical care frameworks like the 7P PROSA model at UZ Leuven [6], active distraction is recognised as one of the most effective non-pharmacological strategies for reducing procedural distress [4]. Research confirms that distraction reliably reduces procedural anxiety, with effect sizes that grow with the depth of attentional engagement [5], [4]. Immersive technologies such as VR extend this principle but are of uncertain suitability for the 3–6 age group due to sensory overload and discomfort with head-mounted displays [7].

What the thesis provided: This project builds on a master's thesis by Thibaut Degreef and Stan Vanherle at KU Leuven [2] that developed a multimodal haptic blanket for paediatric procedural comfort. The thesis platform combines a weighted blanket shown to reduce anticipatory anxiety through deep touch pressure [8], [9], [10] with a 4×4 grid of servo motors for localised pressure, 16 vibrotactile Drake TacHammer actuators, and a PID-controlled thermal layer. Together these stimulate distinct mechanoreceptor populations in the skin [11] and activate C-tactile afferents that are biologically tuned to signal safety and proximity [12], [13]. The result is a passive, caregiver-operated comfort device: soothing, but without active child involvement.

What this course project adds: The limitation of the thesis platform is that the child is a passive recipient. Research shows that active distraction where the child must engage cognitively and physically produces deeper attentional capture than passive sensory input alone [4], [5]. This course project therefore adds a fully interactive game layer to the existing blanket platform, built around a handheld ESP32-based controller. Three age-appropriate games (Snake, Brick Breaker, Balloon Pop) require the child to actively steer, tap, and react, redirecting their attention away from the procedure. Each game is linked in real time to the haptic blanket: game events trigger unique, game-specific haptic patterns across the servo grid and vibrotactile actuators, so the child simultaneously feels the game on their body. Audio feedback through a passive buzzer adds a further attentional anchor, with a distinct melody for each game. The result is a multisensory, child-driven distraction experience in which movement, touch, sound, and vision are all engaged at once.

---

## 2. Hardware / Supplies

Components are divided into two categories: hardware carried over from the existing thesis platform [2] and hardware purchased specifically for this course project.

### 2.1 Hardware Provided by the Thesis Platform

| Component | Quantity | Notes |
|---|---|---|
| ESP32-WROOM microcontroller | 2 | Master and blanket-side controller |
| 4.0" TFT LCD (ILI9488) | 1 | Visual game interface (screen-side unit) |
| PCA9685 PWM driver | 1 | Servo signal generation / control |
| PCA9548A I2C multiplexer | 2 | I2C expansion and TacHammer addressing |
| Titan Haptics Drake TacHammers | 16 | Vibrotactile feedback in the blanket |
| MG90S servos | 16 | Pressure grid on the blanket |
| 5V high-current power supply | 1 | Dedicated actuator power |
| Heating foils (Thermo TECH) | 2 | Thermal comfort layer in blanket |
| NTC thermistors | 8 | Temperature sensing for PID control |
| Medium-density foam layer | 1 | Actuator housing within blanket |
| Weighted blanket (2.6 kg) | 1 | Deep touch pressure and actuator reaction surface |
| Breadboard, jumper wires, USB cables | 1 set | Prototyping and programming |

### 2.2 Hardware Purchased for This Course Project

| Component | Quantity | Notes | Unit Price (approx.) |
|---|---|---|---|
| MPU6050 IMU | 1 | Tilt input for the game controller | €14.27 |
| KY-006 passive buzzer | 1 | Game audio feedback | €1.30 |



---

## 3. Wiring and Setup

The TFT, MPU6050, and buzzer are mounted on the screen-side prototype board. The blanket-side system uses a dedicated 5V power rail for the servos and haptic drivers. The ESP32 is powered separately to avoid brownouts.

### Master ESP32 Pin Assignments

| Signal | GPIO |
|---|---|
| MPU6050 SDA | 21 |
| MPU6050 SCL | 22 |
| TFT MOSI | 23 |
| TFT MISO | 19 |
| TFT SCK | 18 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT RST | 4 |
| TFT BL | 32 |
| Touch CS (XPT2046) | 14 |
| Passive buzzer | 26 |

---

## 4. Methods

### 4.1 System Architecture

The system uses two ESP32 boards:

- **Master node (screen side):** reads the MPU6050, runs the game logic, renders the UI on the TFT screen, and plays audio.
- **Slave node (blanket side):** receives ESP-NOW packets and drives the servos and vibrotactile actuators.

This wireless separation avoids cable strain and keeps the high-current actuator supply isolated from the handheld controller [2]. The architecture mirrors the distributed control structure developed in the thesis, where ESP-NOW was chosen for its low-latency, connectionless peer-to-peer communication over 2.4 GHz without requiring a router or persistent handshake.

![System Architecture Diagram](Images/system_architecture.png)  
*Figure 1: System architecture showing Master (screen-side) and Slave (blanket-side) ESP32 nodes communicating over ESP-NOW.*

### 4.2 Input and Calibration

The MPU6050 is calibrated at startup using a short static sampling phase. During runtime, accelerometer values are low-pass filtered (α = 0.12–0.15) to reduce jitter while preserving responsive tilt control. The gyroscope is intentionally not used at runtime: because the IMU is mounted upside-down inside the handheld unit, fast movements cause pitch and roll axes to cross-contaminate in the body frame. The accelerometer alone provides stable, drift-free absolute orientation.

### 4.3 Game Layer

Three games run on the Master ESP32, all designed to be immediately understandable by children aged 3 to 6 without requiring reading ability or complex instructions. This design principle is consistent with clinical best practice for procedural distraction: a child's full attentional load should be directed at the distractor rather than at learning how to use it [4]. Simple, immediately engaging mechanics maximise the depth of attentional capture at the critical moment of the procedure. A simple state machine manages the games and the manual control screens inherited from the thesis platform.

| Game | Input | Core Mechanic |
|---|---|---|
| Snake | IMU tilt | Navigate the snake to eat food, avoid walls and obstacles |
| Brick Breaker | IMU tilt | Move the paddle to bounce the ball and clear bricks |
| Balloon Pop | Touchscreen | Tap balloons before they float away; avoid bombs |

#### Snake

![Snake Game](Images/game_snake.jpeg)  
*Figure 2: Snake game running on the TFT display. The child tilts the handheld controller to steer the snake toward the red food item.*

#### Brick Breaker

![Brick Breaker Game](Images/game_brickbreaker.jpeg)  
*Figure 3: Brick Breaker game on the TFT display. The child tilts the controller to move the paddle and clear the coloured brick grid.*

#### Balloon Pop

![Balloon Pop Game](Images/game_balloonpop.jpeg)  
*Figure 4: Balloon Pop game on the TFT display. The child taps rising balloons to pop them and must avoid the bomb balloon.*

### 4.4 Haptic Mapping

Game events are translated into haptic patterns on the blanket. Because the physical wiring order of the 16 actuators does not match the visual 4×4 grid, all logical grid positions are routed through a lookup table (`TRIL_MAP`) before transmission. This lookup table was defined in the thesis platform [2] and is carried over unchanged.

The haptic modalities available on the blanket — vibrotactile feedback from the Drake TacHammers and discrete pressure from the servo grid — target different mechanoreceptor populations in the skin. Vibration at higher frequencies activates RAII-LTMRs (Pacinian corpuscles), while servo-driven pressure activates SAI-LTMRs (Merkel cells) [11]. Combining both modalities creates a richer somatosensory signal that is harder to habituate to, increasing the likelihood that the haptic channel holds the child's attention throughout the procedure [12].

#### Snake

Each step of the snake produces two simultaneous haptic signals. A servo pressure gradient, computed with bilinear interpolation across the 4×4 grid, continuously tracks the head position and shifts the pressure peak on the blanket as the snake moves — giving the child a spatial sense of the snake's location. At the same time, a single vibrotactile motor fires at the position of the food item, acting as a spatial hint so the child can feel where the next target is before they see it.

When the snake eats food, all 16 servo motors pulse briefly to 90° and immediately return to zero — a short, full-blanket squeeze that rewards the point. On a collision (self or obstacle), all servos cut to zero and the game-over jingle plays.

#### Brick Breaker

Brick Breaker uses the haptic system in two parallel ways. As the child tilts the controller to move the paddle, the servo grid continuously mirrors the paddle's column position: the column directly below the paddle extends fully while adjacent columns taper off with distance (`intensity = max(0, 1 − dist × 0.7)`), producing a pressure ridge that slides left and right in sync with the paddle. This runs at every update frame and gives a persistent pressure sensation that helps the child feel where the paddle is without looking.

Each time the ball destroys a brick, a single vibrotactile motor fires at the actuator spatially closest to that brick, so the pop is felt in the matching region of the blanket. Clearing all bricks triggers the win jingle and all servos return to zero.

#### Balloon Pop

Balloon Pop is entirely touch-driven. When a balloon is tapped and popped, two haptic events fire together: a vibrotactile motor at the quadrant matching the balloon's screen position activates at full intensity, and the servo gradient function simultaneously drives a pressure peak toward the same location — a localised squeeze-and-buzz that matches where the balloon was popped. Tapping a bomb balloon ends the game immediately: all actuators shut down and the game-over jingle plays.

### 4.5 Audio Feedback

Audio is produced by a KY-006 passive buzzer on GPIO 26, driven through the ESP32's LEDC PWM peripheral. Volume is adjustable from the menu in ten discrete steps (0 = mute, 1–10), mapped to a PWM duty cycle between 3 and 127. All melody playback is non-blocking: the main loop calls `buzzerUpdate()` on every iteration, which advances to the next note only when its duration has elapsed, so audio never stalls the IMU reads, touch polling, or haptic output.

Audio feedback provides an additional attentional anchor beyond the visual and haptic channels. Research on multimodal distraction indicates that pairing auditory and tactile stimulation increases the depth of attentional capture compared to any single modality alone [5], [7]. The non-blocking buzzer architecture was essential for this: because melody updates share the main loop with touch polling and IMU reads, the system stays fully responsive throughout.

Each game has its own looping background melody:

| Game | Melody | Character |
|---|---|---|
| Snake | "Serpentine" (original) | Minor-key, ~150 BPM, E4–E5 range, rolling phrases |
| Brick Breaker | Korobeiniki (Tetris theme) | Upbeat, E major, immediately recognisable |
| Balloon Pop | "Cloud Pop" (original) | Slow, serene, A minor, long note values |

A shared descending game-over jingle (E5 down to E4) plays once at the end of any session, covering both loss conditions and the win condition in Brick Breaker.

### 4.6 Communication

Master and Slave communicate through ESP-NOW. The game sends compact packets containing the actuator state and temperature-related data. A separate temperature channel lets the caregiver monitor the thermal subsystem, which is regulated on the blanket side via closed-loop PID control using thermistors and heating foils [2]. The thermal layer targets a surface temperature around 32–35 °C, which is the range at which C-tactile afferents respond most effectively to touch — the same mechanism that makes skin-temperature human contact feel soothing [12], [13].

---

## 6. Discussion

The prototype demonstrates that a compact embedded system can combine game control, visual feedback, audio, and spatial haptics into a coherent distraction tool for children aged 3 to 6. All three games are deliberately simple, no reading required, no complex rules so that a toddler can engage with them within seconds and a caregiver can hand the device over without explanation. This aligns with clinical best practice for procedural distraction, where a child's full attentional load should be directed at the distractor rather than at learning how to use it [4].

The multimodal design is medically meaningful beyond mere entertainment. The synchronised haptic feedback on the blanket occupies the somatosensory channel, the same channel through which procedural pain (needle insertion, pressure from a blood pressure cuff) is processed. By filling that channel with non-threatening tactile input, the system aims to reduce the salience of the painful stimulus through competitive gating [11], [10]. The audio layer adds an auditory anchor that helps maintain attention even when the child briefly looks away from the screen, which is common at this age [5].

The therapeutic value of the weighted blanket component should not be understated. Clinical evidence shows that deep touch pressure reliably decreases anticipatory anxiety in both adult perioperative settings and paediatric dental care, without adverse cardiovascular effects [8], [9], [10]. For children who undergo repeated procedures such as those in paediatric oncology, the blanket offers a familiar, comforting object they can associate with positive sensory experiences rather than with procedural fear [5].

Among the three games, Brick Breaker produced the most expressive haptic behaviour. The continuously shifting servo gradient under the paddle gives the child a persistent pressure sensation tied to their own physical movement, a form of proprioceptive feedback that is qualitatively different from the event-driven pulses of the other games. This spatial coupling between motor action and blanket response may be particularly effective for the 3–6 age group, where embodied, sensorimotor play is developmentally dominant [3].

The per-game audio melodies added an unexpected layer of identity to each mode. The Tetris melody and the descending game-over jingle are emotionally legible even to very young children — an important property when the child should understand game state without reading text. The non-blocking buzzer architecture was essential: because melody updates share the main loop with touch polling and IMU reads, the system stays fully responsive throughout.

Several technical limitations remain. The passive buzzer produces a thin, monophonic tone. The fixed actuator lookup table requires manual updating if the blanket hardware is revised. ESP-NOW on a shared 2.4 GHz channel is susceptible to interference in hospital environments [2]. Most importantly, the system has not yet been validated with the target user group.

---

## 7. Conclusion and Future Work

This project extends an existing paediatric haptic blanket with an interactive game interface and synchronised multimodal feedback. The result is a technically ambitious prototype that combines tilt-based interaction, wireless communication, haptic mapping, and caregiver-adjustable settings, specifically designed for children aged 3 to 6.

Future improvements include:

- **Formal usability testing with children and clinicians.** Structured play sessions with children aged 3–6, observed by clinicians, would identify which game mechanics are most engaging and which haptic patterns are most comforting [2].
- **Higher-fidelity audio.** Replacing the passive buzzer with a small speaker and an I2S DAC would allow polyphonic melodies and richer sound effects.
- **Additional game modes.** A rhythm game in which the child taps in time with the melody and the blanket pulses on the beat would further exploit the synchronised audio-haptic channel.
- **Adaptive haptic intensity.** A future version could use a lightweight biofeedback signal (e.g. heart rate) to automatically adjust stimulus intensity based on the child's real-time stress level [2].
- **Robust wireless communication.** Migrating to a dedicated frequency band would reduce interference risk in RF-dense hospital environments [2].
- **Clinical integration.** Incorporating the device into structured procedural support frameworks such as the 7P PROSA model at UZ Leuven [6] would position it for formal clinical evaluation alongside trained child life specialists [3].

---

## 8. References

[1] A. Rokach, "Psychological, emotional and physical experiences of hospitalized children," *Clinical Case Reports and Reviews*, vol. 2, no. 4, 2016, doi: 10.15761/CCRR.1000227.

[2] T. Degreef and S. Vanherle, "A Haptic-Centred Multisensory Distraction Device for Reducing Stress in Minor Medical Procedures," master's thesis, KU Leuven, 2026.

[3] J. L. Lerwick, "Minimizing pediatric healthcare-induced anxiety and trauma," *World Journal of Clinical Pediatrics*, vol. 5, no. 2, pp. 143–150, May 2016, doi: 10.5409/wjcp.v5.i2.143.

[4] K. A. Birnie et al., "Systematic Review and Meta-Analysis of Distraction and Hypnosis for Needle-Related Pain and Distress in Children and Adolescents," *Journal of Pediatric Psychology*, vol. 39, no. 8, pp. 783–808, Sep. 2014, doi: 10.1093/jpepsy/jsu029.

[5] I. M. Bukola and D. Paula, "The Effectiveness of Distraction as Procedural Pain Management Technique in Pediatric Oncology Patients: A Meta-analysis and Systematic Review," *Journal of Pain and Symptom Management*, vol. 54, no. 4, pp. 589–600, Oct. 2017, doi: 10.1016/j.jpainsymman.2017.07.006.

[6] UZ Leuven, "Pijn bij kinderen - wat doen we eraan?" (PROSA model). Available: https://www.uzleuven.be/nl/prosa

[7] S. Bernaerts et al., "Virtual Reality for Distraction and Relaxation in a Pediatric Hospital Setting: An Interventional Study With a Mixed-Methods Design," *Frontiers in Digital Health*, vol. 4, p. 866119, May 2022, doi: 10.3389/fdgth.2022.866119.

[8] L. I. Stein Duker, R. McGuire, J. Hernandez, E. Goodman, and J. C. Polido, "Feasibility, acceptability, and perceived effectiveness of weighted blankets during paediatric dental care," *International Journal of Paediatric Dentistry*, Sep. 2024, doi: 10.1111/ipd.13263.

[9] D. R. Payne et al., "Effect of Weighted Blanket Versus Traditional Practices on Anxiety and Pain in Patients Undergoing Elective Surgery: A Multicenter Randomized Controlled Trial," *AORN Journal*, vol. 119, no. 6, pp. 429–439, 2024, doi: 10.1002/aorn.14146.

[10] H.-Y. Chen, "Physiological Effects of Deep Touch Pressure on Anxiety Alleviation: The Weighted Blanket Approach," *Journal of Medical and Biological Engineering*, vol. 33, no. 5, p. 463, 2013, doi: 10.5405/jmbe.1043.

[11] V. E. Abraira and D. D. Ginty, "The Sensory Neurons of Touch," *Neuron*, vol. 79, no. 4, pp. 618–639, Aug. 2013, doi: 10.1016/j.neuron.2013.07.051.

[12] L. S. Löken, J. Wessberg, I. Morrison, F. McGlone, and H. Olausson, "Coding of pleasant touch by unmyelinated afferents in humans," *Nature Neuroscience*, vol. 12, no. 5, pp. 547–548, May 2009, doi: 10.1038/nn.2312.

[13] I. Morrison, "Keep Calm and Cuddle on: Social Touch as a Stress Buffer," *Adaptive Human Behavior and Physiology*, vol. 2, no. 4, pp. 344–362, Aug. 2016, doi: 10.1007/s40750-016-0052-x.
