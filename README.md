# S32K144 Dual-Board CAN Control System

## 1. Overview

This project implements a dual-board embedded control system using two S32K144 microcontrollers communicating over a CAN bus.

The system separates Bluetooth communication and ultrasonic obstacle detection from motor and steering control.

- **Board 1 — Sensing and Communication:** Receives Bluetooth commands from a smartphone, measures obstacle distance using an HC-SR05 ultrasonic sensor, processes the vehicle state, and transmits data over CAN.
- **Board 2 — Vehicle Control:** Receives CAN messages and controls a DC motor through an L298N motor driver and steering angle through a servo motor.

The firmware is developed in C using NXP S32 Design Studio and the S32K144 SDK.

## 2. Hardware Components

### Board 1 — Sensing and Communication

- MaaZEDU Development Board with S32K144
- HC-05 Bluetooth module
- HC-SR05 ultrasonic sensor
- Voltage divider for the ultrasonic ECHO signal
- CAN transceiver

### Board 2 — Motor and Steering Control

- MaaZEDU Development Board with S32K144
- L298N motor driver
- DC motor
- Servo motor
- CAN transceiver

Both boards communicate over a CAN bus.

## 3. System Architecture

```text
                 Smartphone
                      |
                 Bluetooth
                      |
                   HC-05
                      |
                      v
             +----------------+
             |    BOARD 1     |
             |    S32K144     |
             |                |
             | Ultrasonic     |
             | Distance       |
             | Measurement    |
             | State Machine   |
             +----------------+
                      |
                  CAN Bus
                      |
                      v
             +----------------+
             |    BOARD 2     |
             |    S32K144     |
             |                |
             | CAN Receiver   |
             | Motor Control  |
             | Servo Control  |
             +----------------+
                 |         |
                 v         v
              L298N      Servo
                 |
                 v
              DC Motor
```

## 4. Pin Configuration

### Board 1

| Component | Signal | S32K144 Pin |
|---|---|---|
| HC-05 | TXD | PTD6 / LPUART2_RX |
| HC-05 | RXD | PTD7 / LPUART2_TX |
| HC-SR05 | TRIG | PTE0 |
| HC-SR05 | ECHO | PTD17 |
| CAN0 | RX | PTE4 |
| CAN0 | TX | PTE5 |
| Debug UART | TX | PTC7 |

The ultrasonic ECHO signal must pass through a suitable voltage divider before reaching the 3.3 V MCU input.

### Board 2

| Component | Signal | S32K144 Pin |
|---|---|---|
| Servo | PWM signal | PTD10 / FTM2_CH0 |
| L298N | ENA / PWM | PTD5 / FTM2_CH3 |
| L298N | IN1 | PTD17 |
| L298N | IN2 | PTE0 |
| CAN0 | RX | PTE4 |
| CAN0 | TX | PTE5 |
| Debug UART | TX | PTC7 |

The CAN transceivers must connect the MCU CAN interface to the physical CAN bus. Connect CANH to CANH and CANL to CANL, with appropriate bus termination.

## 5. Intended Vehicle Operation

1. The smartphone connects to the HC-05 Bluetooth module.
2. The smartphone sends the `F` command to start the vehicle.
3. Board 1 measures the distance to obstacles using the ultrasonic sensor.
4. When the first obstacle is detected within the configured distance threshold, Board 1 updates the vehicle state and sends a CAN message.
5. Board 2 receives the message and controls the motor and steering servo.
6. The vehicle turns right, continues forward, detects the second obstacle, and turns left.
7. The vehicle continues forward toward the end of the course.

The intended obstacle threshold is **30 cm**.

## 6. CAN Communication

The firmware uses CAN standard identifiers and an 8-byte data frame.

| Byte | Field | Description |
|---|---|---|
| 0 | Start | Vehicle start status |
| 1 | State | Current vehicle state |
| 2 | Distance | Measured distance in centimeters |
| 3 | Obstacle | Obstacle detection flag |
| 4–7 | Reserved | Reserved for future use |

- CAN message ID: `0x100`
- Frame type: Standard CAN
- Data length: 8 bytes
- CAN bitrate: Must match the CAN configuration used by both boards.

A distance value of `255` indicates an invalid or out-of-range measurement in the current firmware.

Board 2 stops the motor if CAN communication times out.

## 7. Firmware Structure

```text
firmware/
├── board1/
│   └── board1_can_pal_main.c
└── board2/
    └── board2_can_pal_main.c
```

The complete S32 Design Studio projects and required SDK configuration files should be included to make the firmware reproducible.

## 8. Development Environment

- NXP S32 Design Studio
- S32K144 SDK
- C programming language
- CAN PAL
- LPUART
- GPIO
- FTM PWM

## 9. Current Implementation Status

The repository contains the initial firmware for both boards.

The following differences must be resolved before claiming that the intended vehicle sequence is fully implemented:

- The current Board 1 firmware uses a 20 cm obstacle threshold; the intended threshold is 30 cm.
- The current state machine commands a right turn at the first obstacle and a left turn at the second obstacle. The intended sequence is right first, then left.
- The end-of-course condition requires a defined completion mechanism. The current firmware does not automatically detect a finish line.

These items must be verified and corrected before the system is considered complete.

## 10. Future Improvements

- Align the firmware with the intended obstacle-avoidance sequence.
- Validate ultrasonic distance measurements.
- Verify CAN communication reliability.
- Calibrate servo steering angles and motor speed.
- Improve fault handling and safe-stop behavior.
- Document hardware testing and validation results.

## 11. Author

Developed as an embedded systems project using two S32K144 microcontrollers.
