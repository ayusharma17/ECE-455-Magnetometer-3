#include <Arduino.h>
#include <limits.h>
#include <string.h>

namespace {

constexpr unsigned long SERIAL_BAUD = 115200;
constexpr size_t COMMAND_BUFFER_SIZE = 96;
constexpr uint32_t MAX_INTERVAL_MS = INT32_MAX;

enum class ControllerState {
  IDLE,
  CONFIGURED,
  RUNNING,
  FAULT,
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

// Development defaults only. Final values depend on the completed hardware.
constexpr MeasurementSettings DEFAULT_SETTINGS = {
    5000, 500, 1000, 1000, 1000, 3000, 10, 20,
};

MeasurementSettings settings;
ControllerState state = ControllerState::IDLE;
uint32_t measurementNumber = 0;
const char *lastFault = "NONE";
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

void printStatus() {
  Serial.print(F("STATUS state="));
  Serial.print(stateName(state));
  Serial.print(F(" measurement="));
  Serial.print(measurementNumber);
  Serial.print(F(" fault="));
  Serial.print(lastFault);
  printSettings();
  Serial.println();
}

void printError(const __FlashStringHelper *code) {
  Serial.print(F("ERR code="));
  Serial.println(code);
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
      printError(F("INVALID_SETTINGS"));
    } else {
      Serial.print(F("PENDING command=START reason=TASK_2 state="));
      Serial.println(stateName(state));
    }
  } else if (strcmp(command, "STOP") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
    } else {
      state = ControllerState::IDLE;
      Serial.println(F("OK command=STOP state=IDLE"));
    }
  } else if (strcmp(command, "DEFAULTS") == 0) {
    if (argument1 != nullptr) {
      printError(F("BAD_COMMAND"));
    } else if (state == ControllerState::RUNNING) {
      printError(F("BUSY"));
    } else {
      settings = DEFAULT_SETTINGS;
      state = ControllerState::CONFIGURED;
      Serial.println(F("OK command=DEFAULTS state=CONFIGURED"));
    }
  } else if (strcmp(command, "SET") == 0) {
    handleSet(argument1, argument2, extra);
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
  settings = DEFAULT_SETTINGS;
  state = ControllerState::IDLE;
  printStatus();
}

void loop() {
  readSerialCommands();
}
