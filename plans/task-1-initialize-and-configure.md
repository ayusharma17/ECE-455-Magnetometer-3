# Task 1: Initialize and Configure

## Goal

Create the Arduino Nano firmware foundation: initialize the controller, load configurable defaults, report status, and accept a measurement request.

## Implementation plan

1. Create an Arduino firmware entry point with `setup()` and `loop()`.
   - Initialize serial at a documented baud rate.
   - Load conservative placeholder defaults, clearly marked as TBD.
   - Start in `IDLE` and emit a machine-readable status response.

2. Define a `MeasurementSettings` structure.
   - Store polarization time, settle time, collection-window length, recycle delay, expected frequency minimum and maximum, and quality thresholds.
   - Validate settings centrally: durations must be nonzero, the minimum frequency must be below the maximum, and values must fit safe integer ranges.

3. Define controller state and status.
   - Initial states: `IDLE`, `CONFIGURED`, `RUNNING`, and `FAULT`.
   - Status should include current state, measurement number, last fault reason, and active settings.
   - `RUNNING` is a placeholder for Task 2, which implements the timing cycle.

4. Implement a minimal serial command interface.
   - `STATUS`: return state and active configuration.
   - `START`: accept a request only when settings are valid and the controller is idle or configured; report that execution is pending until Task 2 is implemented.
   - `SET <field> <value>`: update one setting after validation.
   - `DEFAULTS`: restore default settings.
   - `STOP`: return safely to `IDLE`.
   - Reject unknown or malformed commands without changing settings.

5. Verify manually with Arduino Serial Monitor.
   - Reset reports `IDLE`.
   - Valid settings appear in `STATUS`; invalid settings are rejected.
   - `START` is accepted only from an allowed state.
   - `STOP` always restores `IDLE`.

## Deferred decisions

Do not assign coil-driver pins, final timing values, sensor-input details, boat interface behavior, or storage hardware in this task. Those depend on hardware decisions identified in `DESIGN.md`.

## Design basis

This plan implements the “Initialize and configure” stage in `DESIGN.md`. Its configurable measurement-sequence structure follows the PyPPM reference cited there.
