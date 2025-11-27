# Sound Localization Using Microphone Array for Camera Direction Control

## Project Overview

This project focuses on detecting the direction of a sound source using a microphone array and automatically rotating a servo-mounted camera toward the detected speaker. It is designed for seminars, meetings, webinars, classrooms, and surveillance applications.

## Objectives

* Detect the direction of incoming sound using amplitude comparison.
* Improve signal quality and system stability with filtering techniques.
* Rotate the camera automatically toward the speaker.

## System Components

* Arduino UNO / Microcontroller
* 4 Electret Microphones
* Servo Motor (SG90 / MG995)
* Analog-to-Digital Conversion
* Filtering Circuit
* Camera Mount

## Working Principle

A microphone array captures sound intensity from different angles. The system compares the amplitude of the received signal from each microphone. The microphone with the highest signal strength indicates the closest direction of the sound source. Based on this angle, a servo motor rotates the camera automatically toward that direction.

## Methodology

1. Sound signals are captured using four microphones.
2. Signals are amplified and filtered to remove environmental noise.
3. Arduino reads analog values and applies amplitude comparison logic.
4. The angle of the servo is calculated.
5. Camera rotates toward the detected direction.

## Applications

* Online meetings and live seminars
* Surveillance and security systems
* Smart classrooms and robotics
* Video conferencing

## Expected Outcomes

* Improved speaker focus without manual camera movement
* Real-time automatic camera tracking
* Enhanced audio-visual experience

## Future Scope

* Replace amplitude-based comparison with TDOA (Time Difference of Arrival)
* Integrate face tracking using OpenCV
* Wireless sound direction tracking


## References

* Research papers on microphone array processing
* Arduino documentation
