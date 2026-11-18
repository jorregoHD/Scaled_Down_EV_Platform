# Scaled_Down_EV_Platform

Tabletop version of an Electric Vehicle essential component systems to educate individuals how each component works, and how to diagnose as you would on a full size electric vehicle. 

## T-Display-S3 junction-box trainer

This firmware is a **low-voltage bench simulator only**. It steps through three relay
outputs and shows **DEMO COMPLETE** after the sequence; it does not implement or
indicate a real vehicle READY state.

### Sequence and buttons

Connect the LILYGO T-Display-S3 to USB and flash with PlatformIO:

```text
pio run -t upload
```

- Press **BOOT (GPIO 0)** to advance: precharge on, main negative on, main positive
  on, then precharge off.
- Press **GPIO 14** to reset the sequence. Reset and startup turn all outputs off.
- The screen shows each output state. The completion indicator is labeled
  **DEMO COMPLETE**, not READY.

### Wheel speed display

The display reads one falling edge per wheel revolution from a Hall sensor on
GPIO13. Configure the wheel circumference in `src/Spedometer.cpp` (currently
25 inches). MPH is calculated from wheel RPM and that circumference; it is wheel
speed, not motor-shaft RPM.

The Hall module is powered from 5 V. Do not connect its output directly to GPIO13:
the ESP32-S3 input is not 5 V tolerant. Use a level shifter or a resistor divider
before connecting the signal. For example, connect module OUT through a 10 kOhm
series resistor to GPIO13, then connect a 15 kOhm resistor from GPIO13 to GND.
This scales a 5 V high level to about 3 V. Connect grounds together and confirm
the module's output behavior before powering the circuit.

### Low-voltage relay wiring requirements

- The relay coil is a 5 V load. Do **not** connect it directly to an ESP32 GPIO.
- Each relay needs its own suitable low-side driver and flyback diode across the
  coil. One MOSFET drives only one independent relay output; this sequence needs
  three drivers. Confirm the relay coil current and the driver ratings before
  powering them.
- Add a gate-to-source pull-down so the relay stays off while the ESP32 is
  starting or resetting.
- The sketch defaults to GPIO 10 (precharge), GPIO 11 (main negative), and GPIO 12
  (main positive). These are 3.3 V logic outputs; use a common ground with a
  non-isolated low-side driver.
- The STP110N7F6 must be checked against its datasheet for guaranteed operation
  at a 3.3 V gate drive. Threshold voltage alone does not establish that it will
  switch the coil reliably; use a suitable gate driver or logic-level MOSFET if
  the datasheet does not specify adequate on-resistance at that gate voltage.

Use only a disconnected bench trainer with low-voltage coils or dummy loads. Do
not connect these outputs or relay contacts to an EV, a vehicle contactor, or
any high-voltage circuit.
