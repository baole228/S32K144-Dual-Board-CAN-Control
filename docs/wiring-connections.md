# Wiring Connections

Pin mappings below are taken from the current project source. Confirm the board connector labels against the MaaZEDU board documentation before wiring.

## Board 1

| Device signal | S32K144 pin | Notes |
|---|---|---|
| HC-05 TXD | PTD6 / LPUART2_RX | Bluetooth command input |
| HC-SRF05 TRIG | PTE0 | Trigger output |
| HC-SRF05 ECHO | PTD17 | Connect through a voltage divider to protect the 3.3 V MCU input |
| CAN RX | PTE4 | Connect to the CAN transceiver RXD/TXD side as required by the transceiver |
| CAN TX | PTE5 | Connect through the CAN transceiver |

The current source configures LPUART2 at 9600 baud. Phone commands recognized by the source are `F`, `f`, `1` (run) and `B`, `b`, `2` (stop).

## Board 2

| Device signal | S32K144 pin | Notes |
|---|---|---|
| L298N IN1 | PTD17 | Motor direction |
| L298N IN2 | PTE0 | Motor direction |
| L298N ENA | PTD5 / FTM2_CH3 | PWM speed control |
| Servo signal | PTD10 / FTM2_CH0 | Steering PWM |
| CAN RX | PTE4 | Through CAN transceiver |
| CAN TX | PTE5 | Through CAN transceiver |

## CAN physical layer and power

- Use a CAN transceiver for each board. MCU CAN TX/RX pins are logic-level signals, not CANH/CANL.
- Connect CANH to CANH and CANL to CANL between the transceivers.
- Provide a shared reference ground between the two board systems where required by the transceiver and power arrangement.
- Ensure the two CAN nodes use compatible bit timing (the current source targets 500 kbit/s).
- Do not feed the ultrasonic ECHO signal directly into a 3.3 V MCU input if the sensor outputs a 5 V signal.
- Check the motor and servo supply requirements. Avoid powering a high-current motor or servo from an MCU GPIO pin.
