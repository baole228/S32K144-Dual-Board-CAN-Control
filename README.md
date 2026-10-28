# S32K144 Dual-Board CAN Control

Two NXP S32K144 projects communicate over a CAN bus. Board 1 receives start/stop commands from a phone through an HC-05 Bluetooth module and measures distance using an HC-SRF05 ultrasonic sensor. It sends the run command and measured distance to Board 2. Board 2 receives the CAN frame and controls a DC motor through an L298N driver and a steering servo.

## Repository structure

```text
S32K144-Dual-Board-CAN-Control/
├── README.md
├── .gitignore
├── docs/
│   ├── system-architecture.md
│   ├── wiring-connections.md
│   └── can-protocol.md
└── firmware/
    ├── board1/
    │   └── project_board1_ebs311/
    └── board2/
        └── project_board2_ebs311/
```

The original S32 Design Studio project metadata, source files, board configuration, linker scripts, debugger launch files, and project-local SDK files are retained. Generated `Debug_*` build output folders are omitted because they can be recreated by building the projects.

## Hardware overview

### Board 1 — Bluetooth, ultrasonic sensor, CAN transmitter
- NXP S32K144 MaaZEDU development board
- HC-05 Bluetooth module
- HC-SRF05 ultrasonic sensor
- CAN transceiver connected to the MCU's CAN interface

### Board 2 — CAN receiver, motor and steering control
- NXP S32K144 MaaZEDU development board
- L298N motor driver
- DC motor
- Steering servo
- CAN transceiver connected to the MCU's CAN interface

## Main pin mapping

| Function | Board | S32K144 pin | Notes |
|---|---|---|---|
| HC-05 TXD to MCU RX | Board 1 | PTD6 / LPUART2_RX | Bluetooth command input |
| HC-SRF05 TRIG | Board 1 | PTE0 | Sensor trigger |
| HC-SRF05 ECHO | Board 1 | PTD17 | Use a voltage divider before the 3.3 V MCU input |
| CAN RX | Both boards | PTE4 | Through a CAN transceiver |
| CAN TX | Both boards | PTE5 | Through a CAN transceiver |
| L298N IN1 | Board 2 | PTD17 | Motor direction |
| L298N IN2 | Board 2 | PTE0 | Motor direction |
| L298N ENA/PWM | Board 2 | PTD5 / FTM2_CH3 | Motor speed |
| Servo signal | Board 2 | PTD10 / FTM2_CH0 | Steering PWM |

See [`docs/wiring-connections.md`](docs/wiring-connections.md) for connection notes.

## Operation implemented in the current source

1. The phone sends a start command to the HC-05. Board 1 recognizes `F`, `f`, or `1` as run.
2. Board 1 reads the ultrasonic distance and sends the run command and distance over CAN.
3. Board 2 receives the CAN frame and drives the motor forward.
4. The current Board 2 code detects the first obstacle in the configured 2–80 cm range, steers right, and then straightens the steering.
5. While on the right lane, it detects the second obstacle in the configured 2–70 cm range and steers left to return to the middle lane. A 2.2-second lane-hold timeout is also present.
6. The phone can send `B`, `b`, or `2` to stop. Board 2 stops the motor and centers the servo.

## Important implementation notes

- The current source uses obstacle thresholds of **80 cm for obstacle 1** and **70 cm for obstacle 2**. These do not yet match a 30 cm target.
- After the second lane change, the code continues driving forward; it does not currently implement an independent finish-line detector.
- The CAN payload in the current source is **2 bytes**, not an 8-byte application payload:
  - Byte 0: run command (`1` = run, `0` = stop)
  - Byte 1: distance in centimetres
- CAN uses standard identifier `0x100` and the source configures the CAN controller for 500 kbit/s. Both boards and their CAN transceivers must use compatible settings.
- A CAN transceiver is required between each S32K144 CAN peripheral and the CANH/CANL bus. Do not connect MCU pins directly to CANH/CANL.
- Review the wiring and supply requirements of the motor driver and servo before powering the hardware.

## Import into S32 Design Studio

1. Extract this repository to a local folder.
2. Open S32 Design Studio.
3. Select **File → Import → General → Existing Projects into Workspace**.
4. For the root directory, select `firmware/board1/project_board1_ebs311` and import it.
5. Repeat for `firmware/board2/project_board2_ebs311`.
6. Build each project separately and confirm the SDK/toolchain paths are valid in your S32 Design Studio installation.

If the IDE reports missing SDK references, inspect the project's SDK configuration and select the compatible S32K1xx SDK installed in your environment.

## Repository contents and licensing

The project-local SDK files are included as they appeared in the submitted project archives. Check the applicable NXP SDK license terms before redistributing vendor SDK files publicly. If you publish the repository, include any required third-party notices and choose a repository license only after confirming that it is appropriate for all included content.
