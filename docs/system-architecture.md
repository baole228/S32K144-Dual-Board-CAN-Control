# System Architecture

## Data flow

```text
Phone
  │ Bluetooth
  ▼
HC-05 ── UART ──► Board 1 (S32K144)
                    ├── HC-SRF05: trigger/echo distance measurement
                    └── FlexCAN0 TX (standard ID 0x100)
                               │
                         CAN transceivers
                               │ CANH / CANL
                               ▼
Board 2 (S32K144) ◄── FlexCAN0 RX (standard ID 0x100)
  ├── FTM2_CH3 / PTD5 ──► L298N ENA (motor PWM)
  ├── PTD17 ────────────► L298N IN1
  ├── PTE0 ─────────────► L298N IN2
  └── FTM2_CH0 / PTD10 ─► Steering servo
```

## Responsibilities

- **Board 1:** receives Bluetooth run/stop commands, measures ultrasonic distance, and transmits the run state and distance over CAN.
- **Board 2:** receives the CAN data and controls motor direction, motor PWM, and steering servo according to its obstacle-stage state machine.

## Current behavior in source

Board 2 first changes from the middle lane toward the right when the first distance is within 2–80 cm. It then returns toward the middle lane when the second distance is within 2–70 cm, or when the right-lane timeout is reached. After returning, it continues forward. The source does not include an independent finish-line sensor/detector.
