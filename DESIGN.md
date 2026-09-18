# Software Design

## Role of the code

The firmware coordinates the complete proton-precession measurement cycle, turns the observed signal into a scalar magnetic-field-magnitude reading, decides whether that reading is usable, and reports the result.

The project deck identifies proton precession frequency as proportional to magnetic-field magnitude. The code therefore treats the measured precession frequency as its core input. [Project deck, slide 22]

## Measurement sequence

The measurement sequence is the overall scope of the firmware. It runs one complete magnetic-field measurement from a request through reporting and optional saving of the result.

PyPPM supports programmable measurement sequences rather than one fixed acquisition flow. This design similarly makes signal processing a defined stage within each measurement sequence. [PyPPM]

| Stage | What the code does | Status |
| --- | --- | --- |
| Initialize | Starts the controller, loads default settings, exposes status, and waits for a measurement request. | Can implement now. |
| Configure | Stores measurement settings such as polarization time, settling time, collection-window length, expected frequency range, and quality thresholds. | Can implement now; final values are TBD. |
| Polarize | Commands the polarization coil on for the configured duration. | TBD when the coil driver and its interface are ready. |
| Quench and settle | Commands the polarization current off, then waits for project-defined switching transients to decay before collecting data. | Timing logic can implement now; final control and delay are TBD. |
| Acquire signal | Collects the precession signal during a defined window. | Generated digital test-signal support can implement now; final sensor interface is TBD. |
| Calculate field | Extracts precession frequency from the acquired data and converts it to a field value. | Can implement now using generated test data. |
| Check quality | Decides whether the signal and field estimate are usable. | Can implement now; final thresholds are TBD. |
| Report result | Sends the measurement, quality status, and diagnostic values through the selected operator interface. | Serial reporting can implement now. |
| Save result | Persists measurements if the selected final hardware includes storage. | TBD if storage hardware is selected. |
| Recycle | Waits before accepting the next measurement so the sample and hardware can return to a defined starting condition. | Timing logic can implement now; final delay is TBD. |
| Recover from faults | Stops an incomplete measurement, reports the reason, and returns to an idle state. | Can implement now. |

## Implementation steps

### 1. Initialize and configure

Start the controller, load defaults, expose its status, and accept a measurement request. Store settings for polarization time, quench/settle time, collection-window length, recycle delay, expected frequency range, and quality thresholds. The structure can be written now; the final values are TBD.

### 2. Control the measurement timing

Run polarization, quench/settle, collection, and recycle windows in order. The timing state machine can be implemented now. Quenching means ending the polarization current; the final coil-control behavior and timing values are TBD until the coil driver and sensor hardware are ready. In the final circuit, the controller should assert a logic-level control signal during `POLARIZE` to a properly rated MOSFET or gate driver. A separate power supply and switching stage will provide the coil voltage and current; the Nano must not power the coil directly. The control pin and driver circuit are TBD. When the magnetometer integrates with the boat, this step will wait for a `quiet/ready` signal before collection and return `complete` or `failed` afterward; the interface is TBD. This coordinates measurements with the boat without controlling navigation. [Project deck, slide 20; PyPPM]

### 3. Acquire and calculate

Collect the precession signal during its measurement window, estimate its frequency, and convert that frequency to a field value using the proton gyromagnetic ratio. The generated-signal prototype uses the NIST free-proton value; the final calibration constant remains hardware- and sample-dependent. A generated digital test signal supports initial development of timing, frequency calculation, and the measurement state machine. It does not test the final analog receive path or establish field sensitivity. The final sensor input is TBD: it may provide comparator timing edges or sampled waveform data. Edge-based acquisition will timestamp valid transitions only during the collection window and reject stale or spurious transitions after coil shutoff. [JPM-4; NIST CODATA]

### 4. Validate and recover

Check the result for missing signal, out-of-range frequency, instability, collection timeout, invalid settings, and hardware-interface errors. Report the reason for an invalid measurement, safely stop an incomplete sequence, and return to idle. When a coil driver is connected, fault recovery will force it to its defined safe/off state. These checks reject unreliable readings, but the project target of about 1 nT sensitivity must be demonstrated by the full sensor system; the previous towfish reached a best 9 nT threshold. [Project deck, slide 20]

### 5. Report and save results

Create one record containing the measurement number, settings used, estimated frequency, field value, quality status, and diagnostics. Send the record over serial, retain the latest record in RAM for the `LAST` command, and allow serial commands to start or stop a measurement, change settings, and read status. The RAM record is cleared by reset. Persistent storage, a physical button, and a display are TBD if selected for the final hardware.

## Hardware-dependent decisions

The firmware design leaves the coil-driver pins and safe shutdown behavior, sensor input type, sampling or capture rate, final timing values including recycle delay, quality thresholds, calibration constants, boat quiet/ready interface, and any storage, button, or display interfaces TBD until their hardware is available.

## Current scope

The first implementation will initialize the firmware, accept a generated test signal, calculate the field, apply basic quality checks, report over serial, and recover from missing-signal errors. Coil control, final signal acquisition, and optional storage will be connected when their magnetometer hardware is ready.

## Current platform and future requirements

The current prototype targets an Arduino Nano Every. Its purpose is to develop and test the measurement-sequence logic with a generated input before the magnetometer hardware exists. A disableable RGB status LED may show the sequence during demonstrations; it is not part of the sensing path and should be disabled or located remotely during real low-noise acquisition. [JPM-4]

The final controller remains TBD. It must provide the selected signal-input interface, deterministic measurement timing, a sufficiently stable and calibrated timebase or a way to use an external reference, enough memory for the selected processing method, and interfaces for the final coil driver and any selected storage. The Arduino Nano Every prototype will show which of these requirements exceed its capabilities.

## Magnetometer reference projects

The DIY/reference links come from slide 23 of the supplied [project deck](/Users/ayushsharma/Downloads/Projects_Summaries_FA26.pptx); the project-specific requirements come from slides 20 and 22. Use the relevant source when designing a component.

| Reference | Relevant design use |
| --- | --- |
| [Project deck, slides 20 and 22](/Users/ayushsharma/Downloads/Projects_Summaries_FA26.pptx) | Project-specific 1 nT target, prior towfish baseline, boat quiet requirement, and proton-precession principle. |
| [Proton magnetometer video](https://www.youtube.com/watch?v=VUYyHNzQ2AM&t=95s) | Demonstration reference; inspect its measurement flow before relying on it for a component decision. |
| [IEEE DIY magnetometer](https://spectrum.ieee.org/listen-to-protons-diy-magnetometer) | Simple polarize, quench, and listen sequence. |
| [Ilotresor DIY build](http://ilotresor.com/build-a-proton-precession-magnetometer/) | DIY construction reference; control details remain TBD until inspected. |
| [Steponaitis and Brain paper](https://rla.unc.edu/personal/vps/articles/Steponaitis%20%26%20Brain%201976%20JFA.pdf) | Technical reference to consult when its measurement or signal-processing method applies. |
| [Construction of a Proton Magnetometer](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf) | PIC-controlled timing cycle with quench delay and FFT-based frequency estimation. |
| [Signals from the Subatomic World](https://www.abrazol.com/books/signals/) | DIY build and signal-processing reference. |
| [PyPPM](https://hackaday.io/project/1376-pyppm-a-proton-precession-magnetometer-for-all) | Closest firmware reference: programmable polarization, quench, acquisition, dead-time, and recycle sequence. |
| [JPM-4 design paper](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf) | Configurable cycle timing, precision frequency counting, TCXO reference timing, and quality/sensitivity evaluation. |
| [NIST 2022 CODATA constants](https://physics.nist.gov/cuu/pdf/all.pdf) | Proton gyromagnetic ratio used to convert precession frequency to magnetic-field magnitude. |
