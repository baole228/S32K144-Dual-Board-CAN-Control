# CAN Protocol

The current source uses FlexCAN0 with standard 11-bit identifier `0x100` and a data length of 2 bytes.

| Field | Byte | Meaning |
|---|---:|---|
| Run command | 0 | `1` = run; `0` = stop |
| Ultrasonic distance | 1 | Distance in centimetres, represented as an unsigned 8-bit value |

Board 1 periodically transmits the run command and most recent distance measurement. Board 2 reads the frame and updates its run state and distance.

## Compatibility notes

- Both boards must use the same CAN bitrate and compatible timing configuration. The current source configures 500 kbit/s.
- The protocol currently contains two data bytes; documentation or external tools should not assume an 8-byte application payload.
- Distance values above the sensor's practical range and timeout behavior should be checked against the implementation before relying on a specific sentinel value.
