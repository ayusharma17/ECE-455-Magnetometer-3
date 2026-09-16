# Testing and Demo Guide

This firmware can currently demonstrate configuration, measurement timing, generated-signal field calculation, quality checks, and fault recovery. It does not test a polarization coil, analog receiver, or real sensor input.

## Upload to the Arduino Nano Every

1. Install Arduino IDE 2 from <https://www.arduino.cc/en/software/>.
2. Open **Boards Manager**, search for `Arduino megaAVR Boards`, and install the package.
3. Open `firmware/ProtonMagnetometer/ProtonMagnetometer.ino`.
4. Connect the Nano Every using a data-capable Micro-B USB cable.
5. Select **Tools > Board > Arduino megaAVR Boards > Arduino Nano Every**.
6. Select the new serial port, normally named `/dev/cu.usbmodem...` on macOS.
7. Click **Verify**, then **Upload**.

Do not select the plain **Arduino Nano** board or an `ATmega328P` processor option. Those settings are for the classic Nano, not the Nano Every.

If no USB port appears, try another Micro-B cable before changing drivers or board settings. The power LED only confirms that the cable supplies power; a charge-only or damaged cable can light the board without carrying USB data. Bluetooth and `debug-console` ports are not the Arduino.

## Serial Monitor setup

Open Serial Monitor and select:

- Baud rate: `115200`
- Line ending: `New Line` or `Both NL & CR`

Commands are uppercase. After reset, the controller should report `state=IDLE`, `phase=NONE`, and `fault=NONE`.

## Successful measurement demo

Enter these commands one line at a time:

```text
DEFAULTS
SET polarization_ms 500
SET settle_ms 250
SET collection_ms 500
SET recycle_ms 250
TEST_FAULT NONE
TEST_SIGNAL 2130000 20 1000
START
```

The timing events should appear in this order:

```text
EVENT measurement=1 phase=POLARIZE
EVENT measurement=1 phase=SETTLE
EVENT measurement=1 phase=ACQUIRE
RESULT measurement=1 frequency_millihz=2130000 field_nt=50026 samples=20 spread_millihz=1000 quality=VALID reason=NONE
EVENT measurement=1 phase=RECYCLE
COMPLETE measurement=1 result=REPORTED state=IDLE
```

The measurement number may differ if other measurements were started after reset.

Run `STATUS` afterward. It should show `state=IDLE`, `phase=NONE`, and `fault=NONE`.

## Quality and recovery demonstrations

Keep the short timing settings from the successful demo. Before each case, set the indicated test input and run `START`.

| Case | Setup command | Expected reason |
| --- | --- | --- |
| Missing signal | `TEST_SIGNAL OFF` | `MISSING_SIGNAL` |
| Frequency above configured range | `TEST_SIGNAL 4000000 20 0` | `FREQUENCY_OUT_OF_RANGE` |
| Too few frequency samples | `TEST_SIGNAL 2000000 5 0` | `INSUFFICIENT_SAMPLES` |
| Unstable frequency | `TEST_SIGNAL 2000000 20 50000` | `UNSTABLE_SIGNAL` |
| Simulated collection timeout | `TEST_FAULT COLLECTION_TIMEOUT` | `COLLECTION_TIMEOUT` |
| Simulated hardware error | `TEST_FAULT HARDWARE_INTERFACE` | `HARDWARE_INTERFACE` |

An invalid measurement should produce this pattern:

```text
RESULT ... quality=INVALID reason=<REASON>
FAULT ... reason=<REASON>
RECOVERED ... state=IDLE
```

Run `STATUS` to confirm that the controller recovered to `IDLE` and retained the last fault reason. After testing injected faults, disable the injection with:

```text
TEST_FAULT NONE
```

## Configuration and command checks

These commands should be rejected without changing the active settings:

```text
SET settle_ms 0
SET recycle_ms 4294967295
SET settle_ms abc
STATUS extra
BOGUS
```

Expected errors, in order:

```text
ERR code=INVALID_SETTING
ERR code=INVALID_SETTING
ERR code=BAD_VALUE
ERR code=BAD_COMMAND
ERR code=UNKNOWN_COMMAND
```

To check busy handling, start a longer measurement and immediately try to change a setting:

```text
SET polarization_ms 5000
START
SET settle_ms 100
STOP
```

The `SET` command should return `ERR code=BUSY`. `STOP` should report the measurement as aborted and return the controller to `IDLE`.

## Demo pass criteria

The prototype passes this demo when:

- all four phases occur in order;
- the 2130 Hz test signal reports approximately `50026` nT;
- the valid case reaches `COMPLETE` and returns to `IDLE`;
- every invalid case reports the expected reason and reaches `RECOVERED state=IDLE`;
- invalid or busy commands do not change the active configuration.

Passing these tests verifies the firmware flow only. Final field sensitivity, timing accuracy, calibration, coil shutdown, and real signal acquisition require the completed magnetometer hardware.
