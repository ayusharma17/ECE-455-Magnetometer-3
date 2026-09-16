#include <Arduino.h>
#include <limits.h>
#include <string.h>

namespace {

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr size_t COMMAND_BUFFER_SIZE = 96;
constexpr uint32_t MAX_INTERVAL_MS = INT32_MAX;
constexpr uint32_t PROTON_MILLIHZ_PER_NT_X1E6 = 42577478;
constexpr uint32_t MAX_TEST_SAMPLES = 1000;
constexpr uint8_t RGB_RED_PIN = 5;
constexpr uint8_t RGB_GREEN_PIN = 6;
constexpr uint8_t RGB_BLUE_PIN = 9;
constexpr bool RGB_COMMON_ANODE = false;
constexpr uint32_t FAULT_BLINK_MS = 250;

enum class ControllerState {
  IDLE,
  CONFIGURED,
  RUNNING,
  FAULT,
};

enum class MeasurementPhase {
  NONE,
  POLARIZE,
  SETTLE,
  ACQUIRE,
  RECYCLE,
};

enum class FaultCode {
  NONE,
  INVALID_SETTINGS,
  INVALID_PHASE,
  MISSING_SIGNAL,
  INSUFFICIENT_SAMPLES,
  FREQUENCY_OUT_OF_RANGE,
  UNSTABLE_SIGNAL,
  COLLECTION_TIMEOUT,
  HARDWARE_INTERFACE,
};

struct MeasurementSettings {
  uint32_t polarizationMs;
  uint32_t settleMs;
  uint32_t collectionMs;
  uint32_t recycleMs;
  uint32_t minFrequencyHz;
  uint32_t maxFrequencyHz;
  uint32_t minFrequencySamples;
  uint32_t maxFrequencySpreadHz;
};

struct TestSignalSettings {
  bool enabled;
  uint32_t frequencyMilliHz;
  uint32_t sampleCount;
  uint32_t spreadMilliHz;
};

struct MeasurementResult {
  bool available;
  uint32_t frequencyMilliHz;
  uint32_t fieldNt;
  uint32_t frequencySamples;
  uint32_t frequencySpreadMilliHz;
};

// Development defaults only. Final values depend on the completed hardware.
constexpr MeasurementSettings DEFAULT_SETTINGS = {
    5000, 1000, 2000, 1000, 1000, 3000, 10, 20,
};
constexpr TestSignalSettings DEFAULT_TEST_SIGNAL = {
    true, 2000000, 20, 0,
};

MeasurementSettings settings;
TestSignalSettings testSignal;
MeasurementResult result;
ControllerState state = ControllerState::IDLE;
MeasurementPhase phase = MeasurementPhase::NONE;
uint32_t measurementNumber = 0;
uint32_t phaseStartedAtMs = 0;
FaultCode lastFault = FaultCode::NONE;
FaultCode injectedFault = FaultCode::NONE;
bool demoRgbEnabled = true;
char commandBuffer[COMMAND_BUFFER_SIZE];
size_t commandLength = 0;
bool commandOverflow = false;

const char *stateName(ControllerState value) {
  switch (value) {
    case ControllerState::IDLE:
      return "IDLE";
    case ControllerState::CONFIGURED:
      return "CONFIGURED";
    case ControllerState::RUNNING:
      return "RUNNING";
    case ControllerState::FAULT:
      return "FAULT";
  }
  return "UNKNOWN";
}

const char *phaseName(MeasurementPhase value) {
  switch (value) {
    case MeasurementPhase::NONE:
      return "NONE";
    case MeasurementPhase::POLARIZE:
      return "POLARIZE";
    case MeasurementPhase::SETTLE:
      return "SETTLE";
    case MeasurementPhase::ACQUIRE:
      return "ACQUIRE";
    case MeasurementPhase::RECYCLE:
      return "RECYCLE";
  }
  return "UNKNOWN";
}

const char *faultName(FaultCode value) {
  switch (value) {
    case FaultCode::NONE:
      return "NONE";
    case FaultCode::INVALID_SETTINGS:
      return "INVALID_SETTINGS";
    case FaultCode::INVALID_PHASE:
      return "INVALID_PHASE";
    case FaultCode::MISSING_SIGNAL:
      return "MISSING_SIGNAL";
    case FaultCode::INSUFFICIENT_SAMPLES:
      return "INSUFFICIENT_SAMPLES";
    case FaultCode::FREQUENCY_OUT_OF_RANGE:
      return "FREQUENCY_OUT_OF_RANGE";
    case FaultCode::UNSTABLE_SIGNAL:
      return "UNSTABLE_SIGNAL";
    case FaultCode::COLLECTION_TIMEOUT:
      return "COLLECTION_TIMEOUT";
    case FaultCode::HARDWARE_INTERFACE:
      return "HARDWARE_INTERFACE";
  }
  return "UNKNOWN";
}

void writeRgbChannel(uint8_t pin, bool on) {
  const uint8_t level =
      RGB_COMMON_ANODE ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
  digitalWrite(pin, level);
}

void writeRgb(bool red, bool green, bool blue) {
  writeRgbChannel(RGB_RED_PIN, red);
  writeRgbChannel(RGB_GREEN_PIN, green);
  writeRgbChannel(RGB_BLUE_PIN, blue);
}

void setupRgbIndicator() {
  writeRgb(false, false, false);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
}

void updateRgbIndicator(uint32_t nowMs) {
  if (!demoRgbEnabled) {
    writeRgb(false, false, false);
    return;
  }

  if (state != ControllerState::RUNNING && lastFault != FaultCode::NONE) {
    const bool redOn = (nowMs / FAULT_BLINK_MS) % 2U == 0;
    writeRgb(redOn, false, false);
    return;
  }

  switch (phase) {
    case MeasurementPhase::POLARIZE:
      writeRgb(true, false, false);
      break;
    case MeasurementPhase::SETTLE:
      writeRgb(true, true, false);
      break;
    case MeasurementPhase::ACQUIRE:
      writeRgb(false, false, true);
      break;
    case MeasurementPhase::RECYCLE:
      writeRgb(false, true, false);
      break;
    case MeasurementPhase::NONE:
      writeRgb(false, false, false);
      break;
  }
}

bool settingsAreValid(const MeasurementSettings &candidate) {
  return candidate.polarizationMs > 0 && candidate.settleMs > 0 &&
         candidate.collectionMs > 0 && candidate.recycleMs > 0 &&
         candidate.polarizationMs <= MAX_INTERVAL_MS &&
         candidate.settleMs <= MAX_INTERVAL_MS &&
         candidate.collectionMs <= MAX_INTERVAL_MS &&
         candidate.recycleMs <= MAX_INTERVAL_MS &&
         candidate.minFrequencyHz > 0 &&
         candidate.minFrequencyHz < candidate.maxFrequencyHz &&
         candidate.minFrequencySamples > 0 &&
         candidate.maxFrequencySpreadHz > 0;
}

bool parseUint32(const char *text, uint32_t &value) {
  if (text == nullptr || *text == '\0') {
    return false;
  }

  uint32_t parsed = 0;
  for (const char *cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor < '0' || *cursor > '9') {
      return false;
    }
    const uint8_t digit = static_cast<uint8_t>(*cursor - '0');
    if (parsed > (UINT32_MAX - digit) / 10U) {
      return false;
    }
    parsed = parsed * 10U + digit;
  }

  value = parsed;
  return true;
}

void printSettings() {
  Serial.print(F(" polarization_ms="));
  Serial.print(settings.polarizationMs);
  Serial.print(F(" settle_ms="));
  Serial.print(settings.settleMs);
  Serial.print(F(" collection_ms="));
  Serial.print(settings.collectionMs);
  Serial.print(F(" recycle_ms="));
  Serial.print(settings.recycleMs);
  Serial.print(F(" min_frequency_hz="));
  Serial.print(settings.minFrequencyHz);
  Serial.print(F(" max_frequency_hz="));
  Serial.print(settings.maxFrequencyHz);
  Serial.print(F(" min_frequency_samples="));
  Serial.print(settings.minFrequencySamples);
  Serial.print(F(" max_frequency_spread_hz="));
  Serial.print(settings.maxFrequencySpreadHz);
}

void printTestSignal() {
  Serial.print(F(" test_signal="));
  Serial.print(testSignal.enabled ? F("ON") : F("OFF"));
  Serial.print(F(" test_frequency_millihz="));
  Serial.print(testSignal.frequencyMilliHz);
  Serial.print(F(" test_samples="));
  Serial.print(testSignal.sampleCount);
  Serial.print(F(" test_spread_millihz="));
  Serial.print(testSignal.spreadMilliHz);
}

void printStatus() {
  Serial.print(F("STATUS state="));
  Serial.print(stateName(state));
  Serial.print(F(" measurement="));
  Serial.print(measurementNumber);
  Serial.print(F(" phase="));
  Serial.print(phaseName(phase));
  Serial.print(F(" fault="));
  Serial.print(faultName(lastFault));
  printSettings();
  printTestSignal();
  Serial.print(F(" test_fault="));
  Serial.print(faultName(injectedFault));
  Serial.print(F(" demo_led="));
  Serial.print(demoRgbEnabled ? F("ON") : F("OFF"));
  Serial.println();
}

void printError(const __FlashStringHelper *code) {
  Serial.print(F("ERR code="));
  Serial.println(code);
}

void resetResult() {
  result.available = false;
  result.frequencyMilliHz = 0;
  result.fieldNt = 0;
  result.frequencySamples = 0;
  result.frequencySpreadMilliHz = 0;
}

void acquireTestSignal() {
  resetResult();
  if (!testSignal.enabled || testSignal.sampleCount == 0) {
    return;
  }

  const uint32_t lowerFrequency =
      testSignal.frequencyMilliHz - testSignal.spreadMilliHz / 2U;
  const uint32_t upperFrequency =
      testSignal.frequencyMilliHz +
      (testSignal.spreadMilliHz - testSignal.spreadMilliHz / 2U);
  uint64_t frequencySum = 0;

  for (uint32_t sample = 0; sample < testSignal.sampleCount; ++sample) {
    frequencySum += sample % 2U == 0 ? lowerFrequency : upperFrequency;
  }

  result.available = true;
  result.frequencySamples = testSignal.sampleCount;
  result.frequencyMilliHz = static_cast<uint32_t>(
      (frequencySum + testSignal.sampleCount / 2U) / testSignal.sampleCount);
  result.frequencySpreadMilliHz = upperFrequency - lowerFrequency;
  // Prototype conversion: 2022 CODATA free-proton value. Calibration is TBD.
  result.fieldNt = static_cast<uint32_t>(
      (static_cast<uint64_t>(result.frequencyMilliHz) * 1000000ULL +
       PROTON_MILLIHZ_PER_NT_X1E6 / 2U) /
      PROTON_MILLIHZ_PER_NT_X1E6);
}

void printResult(FaultCode fault) {
  Serial.print(F("RESULT measurement="));
  Serial.print(measurementNumber);
  Serial.print(F(" frequency_millihz="));
  Serial.print(result.frequencyMilliHz);
  Serial.print(F(" field_nt="));
  Serial.print(result.fieldNt);
  Serial.print(F(" samples="));
  Serial.print(result.frequencySamples);
  Serial.print(F(" spread_millihz="));
  Serial.print(result.frequencySpreadMilliHz);
  Serial.print(F(" quality="));
  Serial.print(fault == FaultCode::NONE ? F("VALID") : F("INVALID"));
  Serial.print(F(" reason="));
  Serial.println(faultName(fault));
}

FaultCode validateResult() {
  if (injectedFault != FaultCode::NONE) {
    return injectedFault;
  }
  if (!result.available) {
    return FaultCode::MISSING_SIGNAL;
  }
  if (result.frequencySamples < settings.minFrequencySamples) {
    return FaultCode::INSUFFICIENT_SAMPLES;
  }

  const uint64_t minimumMilliHz =
      static_cast<uint64_t>(settings.minFrequencyHz) * 1000ULL;
  const uint64_t maximumMilliHz =
      static_cast<uint64_t>(settings.maxFrequencyHz) * 1000ULL;
  if (result.frequencyMilliHz < minimumMilliHz ||
      result.frequencyMilliHz > maximumMilliHz) {
    return FaultCode::FREQUENCY_OUT_OF_RANGE;
  }
  if (result.frequencySpreadMilliHz >
      static_cast<uint64_t>(settings.maxFrequencySpreadHz) * 1000ULL) {
    return FaultCode::UNSTABLE_SIGNAL;
  }
  return FaultCode::NONE;
}

void recoverFromFault(FaultCode fault) {
  state = ControllerState::FAULT;
  phase = MeasurementPhase::NONE;
  lastFault = fault;
  Serial.print(F("FAULT measurement="));
  Serial.print(measurementNumber);
  Serial.print(F(" reason="));
  Serial.println(faultName(fault));

  // Coil shutdown will be added here when its hardware interface is defined.
  state = ControllerState::IDLE;
  Serial.print(F("RECOVERED measurement="));
  Serial.print(measurementNumber);
  Serial.println(F(" state=IDLE"));
}

void enterPhase(MeasurementPhase nextPhase, uint32_t nowMs) {
  phase = nextPhase;
  phaseStartedAtMs = nowMs;
  Serial.print(F("EVENT measurement="));
  Serial.print(measurementNumber);
  Serial.print(F(" phase="));
  Serial.println(phaseName(phase));
}

uint32_t phaseDurationMs() {
  switch (phase) {
    case MeasurementPhase::POLARIZE:
      return settings.polarizationMs;
    case MeasurementPhase::SETTLE:
      return settings.settleMs;
    case MeasurementPhase::ACQUIRE:
      return settings.collectionMs;
    case MeasurementPhase::RECYCLE:
      return settings.recycleMs;
    case MeasurementPhase::NONE:
      return 0;
  }
  return 0;
}

void updateMeasurement(uint32_t nowMs) {
  if (state != ControllerState::RUNNING ||
      static_cast<uint32_t>(nowMs - phaseStartedAtMs) < phaseDurationMs()) {
    return;
  }

  switch (phase) {
    case MeasurementPhase::POLARIZE:
      enterPhase(MeasurementPhase::SETTLE, nowMs);
      break;
    case MeasurementPhase::SETTLE:
      enterPhase(MeasurementPhase::ACQUIRE, nowMs);
      break;
    case MeasurementPhase::ACQUIRE:
      if (injectedFault == FaultCode::NONE) {
        acquireTestSignal();
      } else {
        resetResult();
      }
      {
        const FaultCode fault = validateResult();
        printResult(fault);
        if (fault == FaultCode::NONE) {
          enterPhase(MeasurementPhase::RECYCLE, nowMs);
        } else {
          recoverFromFault(fault);
        }
      }
      break;
    case MeasurementPhase::RECYCLE:
      state = ControllerState::IDLE;
      phase = MeasurementPhase::NONE;
      Serial.print(F("COMPLETE measurement="));
      Serial.print(measurementNumber);
      Serial.println(F(" result=REPORTED state=IDLE"));
      break;
    case MeasurementPhase::NONE:
      recoverFromFault(FaultCode::INVALID_PHASE);
      break;
  }
}

void handleTestFault(char *faultText, char *extra) {
  if (state == ControllerState::RUNNING) {
    printError(F("BUSY"));
    return;
  }
  if (faultText == nullptr || extra != nullptr) {
    printError(F("BAD_COMMAND"));
    return;
  }

  if (strcmp(faultText, "NONE") == 0) {
    injectedFault = FaultCode::NONE;
  } else if (strcmp(faultText, "COLLECTION_TIMEOUT") == 0) {
    injectedFault = FaultCode::COLLECTION_TIMEOUT;
  } else if (strcmp(faultText, "HARDWARE_INTERFACE") == 0) {
    injectedFault = FaultCode::HARDWARE_INTERFACE;
  } else {
    printError(F("INVALID_TEST_FAULT"));
    return;
  }

  Serial.print(F("OK command=TEST_FAULT fault="));
  Serial.println(faultName(injectedFault));
}

void handleDemoLed(char *modeText, char *extra) {
  if (state == ControllerState::RUNNING) {
    printError(F("BUSY"));
    return;
  }
  if (modeText == nullptr || extra != nullptr) {
    printError(F("BAD_COMMAND"));
    return;
  }

  if (strcmp(modeText, "ON") == 0) {
    demoRgbEnabled = true;
  } else if (strcmp(modeText, "OFF") == 0) {
    demoRgbEnabled = false;
    writeRgb(false, false, false);
  } else {
    printError(F("INVALID_DEMO_LED"));
    return;
  }

  Serial.print(F("OK command=DEMO_LED mode="));
  Serial.println(demoRgbEnabled ? F("ON") : F("OFF"));
}

bool testSignalSettingsAreValid(const TestSignalSettings &candidate) {
  const uint32_t upperOffset =
      candidate.spreadMilliHz - candidate.spreadMilliHz / 2U;
  return candidate.frequencyMilliHz > 0 && candidate.sampleCount > 0 &&
         candidate.sampleCount <= MAX_TEST_SAMPLES &&
         candidate.spreadMilliHz <= candidate.frequencyMilliHz &&
         candidate.frequencyMilliHz <= UINT32_MAX - upperOffset;
}

void handleTestSignal(char *frequencyText, char *sampleCountText,
                      char *spreadText, char *extra) {
  if (state == ControllerState::RUNNING) {
    printError(F("BUSY"));
    return;
  }
  if (frequencyText != nullptr && strcmp(frequencyText, "OFF") == 0 &&
      sampleCountText == nullptr) {
    testSignal.enabled = false;
    Serial.println(F("OK command=TEST_SIGNAL mode=OFF"));
    return;
  }
  if (frequencyText == nullptr || sampleCountText == nullptr ||
      spreadText == nullptr || extra != nullptr) {
    printError(F("BAD_COMMAND"));
    return;
  }

  TestSignalSettings candidate = testSignal;
  if (!parseUint32(frequencyText, candidate.frequencyMilliHz) ||
      !parseUint32(sampleCountText, candidate.sampleCount) ||
      !parseUint32(spreadText, candidate.spreadMilliHz)) {
    printError(F("BAD_VALUE"));
    return;
  }
  candidate.enabled = true;
  if (!testSignalSettingsAreValid(candidate)) {
    printError(F("INVALID_TEST_SIGNAL"));
    return;
  }

  testSignal = candidate;
  Serial.print(F("OK command=TEST_SIGNAL frequency_millihz="));
  Serial.print(testSignal.frequencyMilliHz);
  Serial.print(F(" samples="));
  Serial.print(testSignal.sampleCount);
  Serial.print(F(" spread_millihz="));
  Serial.println(testSignal.spreadMilliHz);
}

bool updateSetting(const char *field, uint32_t value) {
  MeasurementSettings candidate = settings;

  if (strcmp(field, "polarization_ms") == 0) {
    candidate.polarizationMs = value;
  } else if (strcmp(field, "settle_ms") == 0) {
    candidate.settleMs = value;
  } else if (strcmp(field, "collection_ms") == 0) {
    candidate.collectionMs = value;
  } else if (strcmp(field, "recycle_ms") == 0) {
    candidate.recycleMs = value;
  } else if (strcmp(field, "min_frequency_hz") == 0) {
    candidate.minFrequencyHz = value;
  } else if (strcmp(field, "max_frequency_hz") == 0) {
    candidate.maxFrequencyHz = value;
  } else if (strcmp(field, "min_frequency_samples") == 0) {
    candidate.minFrequencySamples = value;
  } else if (strcmp(field, "max_frequency_spread_hz") == 0) {
    candidate.maxFrequencySpreadHz = value;
  } else {
    return false;
  }

  if (!settingsAreValid(candidate)) {
    return false;
  }

  settings = candidate;
  state = ControllerState::CONFIGURED;
  return true;
}

void handleSet(char *field, char *valueText, char *extra) {
  if (state == ControllerState::RUNNING) {
    printError(F("BUSY"));
    return;
  }
  if (field == nullptr || valueText == nullptr || extra != nullptr) {
    printError(F("BAD_COMMAND"));
    return;
  }

  uint32_t value = 0;
  if (!parseUint32(valueText, value)) {
    printError(F("BAD_VALUE"));
    return;
  }
  if (!updateSetting(field, value)) {
    printError(F("INVALID_SETTING"));
    return;
  }

  Serial.print(F("OK command=SET field="));
  Serial.print(field);
  Serial.print(F(" value="));
  Serial.println(value);
}

void handleCommand(char *line) {
  char *savePointer = nullptr;
  char *command = strtok_r(line, " \t", &savePointer);
  char *argument1 = strtok_r(nullptr, " \t", &savePointer);
  char *argument2 = strtok_r(nullptr, " \t", &savePointer);
  char *argument3 = strtok_r(nullptr, " \t", &savePointer);
  char *extra = strtok_r(nullptr, " \t", &savePointer);

  if (command == nullptr) {
    return;
  }

  if (strcmp(command, "STATUS") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
      return;
    }
    printStatus();
  } else if (strcmp(command, "START") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
    } else if (state != ControllerState::IDLE &&
               state != ControllerState::CONFIGURED) {
      printError(F("NOT_READY"));
    } else if (!settingsAreValid(settings)) {
      recoverFromFault(FaultCode::INVALID_SETTINGS);
    } else {
      ++measurementNumber;
      state = ControllerState::RUNNING;
      lastFault = FaultCode::NONE;
      Serial.print(F("OK command=START measurement="));
      Serial.println(measurementNumber);
      enterPhase(MeasurementPhase::POLARIZE, millis());
    }
  } else if (strcmp(command, "STOP") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
    } else {
      if (state == ControllerState::RUNNING) {
        Serial.print(F("ABORTED measurement="));
        Serial.print(measurementNumber);
        Serial.println(F(" reason=OPERATOR_STOP"));
      }
      state = ControllerState::IDLE;
      phase = MeasurementPhase::NONE;
      Serial.println(F("OK command=STOP state=IDLE"));
    }
  } else if (strcmp(command, "DEFAULTS") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
    } else if (state == ControllerState::RUNNING) {
      printError(F("BUSY"));
    } else {
      settings = DEFAULT_SETTINGS;
      testSignal = DEFAULT_TEST_SIGNAL;
      lastFault = FaultCode::NONE;
      injectedFault = FaultCode::NONE;
      demoRgbEnabled = true;
      resetResult();
      state = ControllerState::CONFIGURED;
      Serial.println(F("OK command=DEFAULTS state=CONFIGURED"));
    }
  } else if (strcmp(command, "SET") == 0) {
    handleSet(argument1, argument2, argument3);
  } else if (strcmp(command, "TEST_SIGNAL") == 0) {
    handleTestSignal(argument1, argument2, argument3, extra);
  } else if (strcmp(command, "TEST_FAULT") == 0) {
    handleTestFault(argument1, argument2);
  } else if (strcmp(command, "DEMO_LED") == 0) {
    handleDemoLed(argument1, argument2);
  } else {
    printError(F("UNKNOWN_COMMAND"));
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\n' || incoming == '\r') {
      if (commandOverflow) {
        printError(F("COMMAND_TOO_LONG"));
      } else if (commandLength > 0) {
        commandBuffer[commandLength] = '\0';
        handleCommand(commandBuffer);
      }
      commandLength = 0;
      commandOverflow = false;
    } else if (!commandOverflow) {
      if (commandLength < COMMAND_BUFFER_SIZE - 1) {
        commandBuffer[commandLength++] = incoming;
      } else {
        commandOverflow = true;
      }
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  setupRgbIndicator();
  settings = DEFAULT_SETTINGS;
  testSignal = DEFAULT_TEST_SIGNAL;
  resetResult();
  state = ControllerState::IDLE;
  phase = MeasurementPhase::NONE;
  lastFault = FaultCode::NONE;
  injectedFault = FaultCode::NONE;
  demoRgbEnabled = true;
  printStatus();
}

void loop() {
  readSerialCommands();
  const uint32_t nowMs = millis();
  updateMeasurement(nowMs);
  updateRgbIndicator(nowMs);
}
