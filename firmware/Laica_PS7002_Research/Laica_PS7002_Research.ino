/*
 * Laica PS7002 / YoHealth BLE research logger
 *
 * Purpose
 * -------
 * Passively listen for the final BLE advertising frame produced by a
 * Laica PS7002 Smart scale, decode the measured weight and the raw
 * bio-impedance/"health" value, then reproduce the body-composition
 * calculations recovered from the historical YoHealth native library.
 *
 * This sketch is intentionally verbose and research-oriented.  It is not
 * intended to be a medical device and the derived body-composition values
 * must be treated as consumer-scale estimates.
 *
 * License: MIT - see repository LICENSE.
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <math.h>
#include <string.h>
#include <string>

// -----------------------------------------------------------------------------
// User profile
// -----------------------------------------------------------------------------
// These values MUST match the profile configured in the Laica application
// during comparison tests.
//
// Current validation profile:
//   male, born 1970-12-10, age 55 on 2026-09-18, height 175 cm.
//
// Age is deliberately stored explicitly.  Consumer BIA algorithms use age as
// an input, so after a birthday the value must be updated to keep comparisons
// with the application meaningful.

static constexpr bool    PROFILE_MALE      = true;
static constexpr uint8_t PROFILE_AGE_YEARS = 55;
static constexpr float   PROFILE_HEIGHT_CM = 175.0f;

// -----------------------------------------------------------------------------
// BLE scanner configuration
// -----------------------------------------------------------------------------

static constexpr uint32_t SERIAL_BAUD = 115200;

// The observed PS7002 advertising is non-connectable and already includes its
// local name in the same advertising payload, so active scan is unnecessary.
static constexpr bool ACTIVE_SCAN = false;

// NimBLE must pass repeated advertisements to us.  We suppress duplicates at
// the measurement/session layer instead of at the BLE controller layer.
static constexpr bool SCAN_DUPLICATES = true;

// A new measurement session is armed after this amount of radio silence from
// the scale.  This allows an identical weight/impedance result to be accepted
// again in a later weighing session.
static constexpr uint32_t SESSION_GAP_MS = 5000;

// Restart scanner periodically.  The onScanEnd callback starts it again.
static constexpr uint32_t SCAN_PERIOD_MS = 30000;

// -----------------------------------------------------------------------------
// Protocol constants recovered from captures
// -----------------------------------------------------------------------------
// NimBLE getManufacturerData() includes the two-byte BLE Company Identifier.
// The observed packet is:
//
//   02 A1 09 FF WW WW ZZ ZZ SS FF FF 21 CC AA
//   ----- ----- ----- ----- -- ----- -- -- --
//     |     |     |     |    |   |   |  |  +-- terminator 0xAA
//     |     |     |     |    |   |   |  +----- checksum
//     |     |     |     |    |   |   +-------- constant 0x21 observed
//     |     |     |     |    |   +------------ reserved/unknown FF FF
//     |     |     |     |    +---------------- status flags
//     |     |     |     +--------------------- health/impedance, BE
//     |     |     +--------------------------- weight x10, BE
//     |     +--------------------------------- YoHealth header 09 FF
//     +--------------------------------------- company id bytes 02 A1
//
// Confirmed final body-composition frame observed on PS7002:
//   status = 0x86 and health/impedance != 0xFFFF.

static constexpr size_t  MFG_LEN = 14;
static constexpr uint8_t STATUS_MEASURING       = 0x80;
static constexpr uint8_t STATUS_WEIGHT_STABLE   = 0x82;
static constexpr uint8_t STATUS_BODY_COMPLETE   = 0x86;

// -----------------------------------------------------------------------------
// Result structure
// -----------------------------------------------------------------------------

struct BodyMetrics {
  float weightKg;
  uint16_t impedance;

  // Values whose semantics are confirmed by comparison with the application.
  float bmi;
  float leanMassKg;
  float bodyFatPct;
  float waterPct;
  float musclePct;
  int   bmrKcal;
  int   bodyAge;

  // Strong hypothesis.  The equation yields a physically plausible value in
  // kg but its exact UI mapping still needs confirmation from app comparisons.
  float boneCandidateKg;

  // Two additional quantities present in the native routine.  Their formulas
  // are recovered, but their semantic names are not yet confirmed.  Logging
  // them is useful if the app exposes additional fields in other versions.
  float nativeMetricX;
  float nativeMetricY;
};

// -----------------------------------------------------------------------------
// Session state
// -----------------------------------------------------------------------------

static uint32_t lastYoHealthSeenMs = 0;
static bool sessionActive = false;
static bool finalMeasurementEmitted = false;
static uint32_t measurementCounter = 0;
static uint8_t lastObservedMfg[MFG_LEN];
static bool haveLastObservedMfg = false;

// -----------------------------------------------------------------------------
// Small helpers
// -----------------------------------------------------------------------------

static uint16_t readBE16(const uint8_t *p) {
  return ((uint16_t)p[0] << 8) | p[1];
}

static void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

static void printHex(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    printHexByte(data[i]);
    if (i + 1 < len) Serial.print(' ');
  }
}

static uint8_t protocolChecksum(const uint8_t *data) {
  // Confirmed against captured PS7002 frames: checksum is the low 8 bits of
  // the sum of manufacturer-data bytes 0..11. Byte 12 contains the checksum.
  uint16_t sum = 0;
  for (size_t i = 0; i <= 11; ++i) sum += data[i];
  return (uint8_t)(sum & 0xFF);
}

static bool checksumValid(const uint8_t *data) {
  return protocolChecksum(data) == data[12];
}

static bool isYoHealthPacket(const uint8_t *data, size_t len) {
  if (len != MFG_LEN) return false;
  if (data[0] != 0x02 || data[1] != 0xA1) return false;
  if (data[2] != 0x09 || data[3] != 0xFF) return false;
  if (data[13] != 0xAA) return false;
  return true;
}

static const char *statusName(uint8_t status) {
  switch (status) {
    case STATUS_MEASURING:     return "MEASURING/NO_IMPEDANCE";
    case STATUS_WEIGHT_STABLE: return "WEIGHT_STABLE";
    case STATUS_BODY_COMPLETE: return "BODY_COMPLETE";
    default:                   return "UNKNOWN";
  }
}

// -----------------------------------------------------------------------------
// Recovered YoHealth calculation
// -----------------------------------------------------------------------------
// The formulas below were reconstructed from the historical native
// libyohealth.so routine named getHealth().  Confirmed values for the reference
// measurement are:
//
//   male, age 55, height 175 cm, weight 80.7 kg, impedance 665
//   BMI       -> 26.35
//   body fat  -> 23.286... %  (app: 23.28 %)
//   water     -> 56.001... %  (app: 56 %)
//   muscle    -> 38.730... %  (app: 38.73 %)
//
// The native code also calculates BMR, body age and two extra metrics.
// Bone mapping remains an explicit validation target.

static BodyMetrics calculateBodyMetrics(float weightKg, uint16_t impedance) {
  BodyMetrics m{};

  const float age = (float)PROFILE_AGE_YEARS;
  const float h = PROFILE_HEIGHT_CM;
  const float sex = PROFILE_MALE ? 0.0f : 1.0f; // native routine: 0 male, 1 female
  const float heightM = h / 100.0f;

  m.weightKg = weightKg;
  m.impedance = impedance;
  m.bmi = weightKg / (heightM * heightM);

  // Lean body mass estimate recovered from getHealth().
  m.leanMassKg =
      0.00067f * h * h
      + 2.0f
      + 0.53f * weightKg
      - 0.00095f * (float)impedance
      - 3.0f * sex
      - 0.05f * age;

  // Body-fat fraction.  The original code applies a low-fat correction below
  // 10%.  This correction is retained exactly because it is part of the
  // recovered implementation rather than a new empirical adjustment.
  float fatFraction = (weightKg - m.leanMassKg) / weightKg;
  if (fatFraction < 0.10f) {
    fatFraction = fatFraction + 0.7f * (0.10f - fatFraction);
  }
  m.bodyFatPct = fatFraction * 100.0f;

  // Total-body-water percentage is 73% of estimated lean mass, normalized by
  // body weight.  This reproduces the app result for the reference capture.
  const float waterFraction = 0.73f * m.leanMassKg / weightKg;
  m.waterPct = waterFraction * 100.0f;

  if (PROFILE_MALE) {
    // Male branch recovered from native code.
    m.musclePct =
        (((7.78f * h + 334.0f - 9.8f * age) / weightKg) + 24.4f);

    // Harris-Benedict-style BMR equation used by the native routine.
    m.bmrKcal = (int)lroundf(
        13.7f * weightKg + 5.0f * h - 6.8f * age + 66.0f);
  } else {
    // Female branch recovered from native code.
    m.musclePct =
        (((7.74f * h - 318.0f - 9.8f * age) / weightKg) + 24.4f);

    m.bmrKcal = (int)lroundf(
        9.6f * weightKg + 1.8f * h - 4.7f * age + 655.0f);
  }

  // Candidate bone-mass equation.  The numeric equation is recovered from the
  // native routine and produces a plausible kg value.  The exact mapping to
  // the Laica UI still has to be confirmed experimentally.
  float boneBase =
      0.0077200001f * weightKg
      + 0.0045f * h
      + 1.95f
      - 0.00636f * age
      - 0.000232f * (float)impedance;

  if (!PROFILE_MALE) {
    // Female path in the native routine applies a 0.75 multiplier.
    boneBase *= 0.75f;
  }
  m.boneCandidateKg = boneBase;

  // The native routine subsequently multiplies the previous term by 30 before
  // formatting one of its return fields.  Its user-facing semantic meaning is
  // not yet known, therefore it is deliberately kept under a neutral name.
  m.nativeMetricX = 30.0f * boneBase;

  // Another return field is a sex-dependent fraction of body-fat fraction.
  // Male coefficient 0.45, female coefficient 0.20.  Again the formula is
  // recovered but the semantic field name is pending validation.
  m.nativeMetricY = fatFraction * (PROFILE_MALE ? 0.45f : 0.20f);

  // "Body age" heuristic recovered from the final native-code branch.
  const int ageInt = PROFILE_AGE_YEARS;
  if (ageInt < 20) {
    m.bodyAge = ageInt;
  } else if (m.bmi > 28.0f) {
    m.bodyAge = ageInt + 15;
  } else if (m.bmi > 26.0f) {
    m.bodyAge = ageInt + 12;
  } else if (m.bmi > 25.0f) {
    m.bodyAge = ageInt + 7;
  } else if (m.bmi > 23.0f) {
    m.bodyAge = ageInt + 4;
  } else if (ageInt < 30) {
    m.bodyAge = 18;
  } else if (ageInt <= 44) {
    m.bodyAge = ageInt - 12;
  } else {
    m.bodyAge = ageInt - 16;
  }

  return m;
}

// -----------------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------------

static void printFinalMeasurement(const NimBLEAdvertisedDevice *dev,
                                  const uint8_t *data,
                                  float weightKg,
                                  uint16_t impedance) {
  const BodyMetrics m = calculateBodyMetrics(weightKg, impedance);
  ++measurementCounter;

  Serial.println();
  Serial.println("============================================================");
  Serial.printf("FINAL MEASUREMENT #%lu\n", (unsigned long)measurementCounter);
  Serial.println("============================================================");
  Serial.printf("RSSI                  : %d dBm\n", dev->getRSSI());
  Serial.print ("Manufacturer data     : "); printHex(data, MFG_LEN); Serial.println();
  Serial.printf("Checksum              : 0x%02X [OK]\n", data[12]);
  Serial.printf("Status                : 0x%02X (%s)\n", data[8], statusName(data[8]));
  Serial.println();
  Serial.printf("Weight                : %.2f kg\n", m.weightKg);
  Serial.printf("Impedance / health    : %u\n", m.impedance);
  Serial.printf("BMI                   : %.2f\n", m.bmi);
  Serial.printf("Lean mass (internal)  : %.3f kg\n", m.leanMassKg);
  Serial.printf("Body fat              : %.2f %%\n", m.bodyFatPct);
  Serial.printf("Water                 : %.2f %%\n", m.waterPct);
  Serial.printf("Muscle                : %.2f %%\n", m.musclePct);
  Serial.printf("BMR                   : %d kcal/day\n", m.bmrKcal);
  Serial.printf("Body age              : %d\n", m.bodyAge);
  Serial.printf("Bone candidate        : %.3f kg  [TO VALIDATE]\n", m.boneCandidateKg);
  Serial.printf("Native metric X       : %.3f      [SEMANTICS UNKNOWN]\n", m.nativeMetricX);
  Serial.printf("Native metric Y       : %.4f     [SEMANTICS UNKNOWN]\n", m.nativeMetricY);
  Serial.println();
  Serial.println("CSV_HEADER,seq,weight_kg,impedance,bmi,lean_kg,fat_pct,water_pct,muscle_pct,bone_candidate_kg,bmr_kcal,body_age,native_x,native_y,rssi,mfg");
  Serial.print("CSV,");
  Serial.print(measurementCounter); Serial.print(',');
  Serial.print(m.weightKg, 2); Serial.print(',');
  Serial.print(m.impedance); Serial.print(',');
  Serial.print(m.bmi, 4); Serial.print(',');
  Serial.print(m.leanMassKg, 4); Serial.print(',');
  Serial.print(m.bodyFatPct, 4); Serial.print(',');
  Serial.print(m.waterPct, 4); Serial.print(',');
  Serial.print(m.musclePct, 4); Serial.print(',');
  Serial.print(m.boneCandidateKg, 4); Serial.print(',');
  Serial.print(m.bmrKcal); Serial.print(',');
  Serial.print(m.bodyAge); Serial.print(',');
  Serial.print(m.nativeMetricX, 4); Serial.print(',');
  Serial.print(m.nativeMetricY, 6); Serial.print(',');
  Serial.print(dev->getRSSI()); Serial.print(',');
  for (size_t i = 0; i < MFG_LEN; ++i) printHexByte(data[i]);
  Serial.println();
  Serial.println("============================================================");
}

// -----------------------------------------------------------------------------
// Packet processing
// -----------------------------------------------------------------------------

static void processYoHealth(const NimBLEAdvertisedDevice *dev,
                            const uint8_t *data) {
  const uint32_t now = millis();

  // A large gap means this is a new weighing session.
  if (!sessionActive || (uint32_t)(now - lastYoHealthSeenMs) > SESSION_GAP_MS) {
    sessionActive = true;
    finalMeasurementEmitted = false;
    haveLastObservedMfg = false;
    Serial.println();
    Serial.println("[YoHealth] New measurement session detected");
  }
  lastYoHealthSeenMs = now;

  if (!checksumValid(data)) {
    Serial.printf("[YoHealth] Ignored packet: checksum fail (rx=%02X calc=%02X)\n",
                  data[12], protocolChecksum(data));
    return;
  }

  const uint16_t weightRaw = readBE16(data + 4);
  const uint16_t healthRaw = readBE16(data + 6);
  const uint8_t status = data[8];
  const float weightKg = weightRaw / 10.0f;

  // Print only when the actual manufacturer-data payload changes.  The scale
  // repeats each advertising frame many times, so this keeps the research log
  // compact while still showing every transition in weight/status/impedance.
  const bool payloadChanged =
      !haveLastObservedMfg || memcmp(lastObservedMfg, data, MFG_LEN) != 0;
  if (payloadChanged) {
    memcpy(lastObservedMfg, data, MFG_LEN);
    haveLastObservedMfg = true;
    Serial.printf("[YoHealth] weight=%.1f kg health=", weightKg);
    if (healthRaw == 0xFFFF) Serial.print("NA");
    else Serial.print(healthRaw);
    Serial.printf(" status=0x%02X %s\n", status, statusName(status));
  }

  // A body-composition result is accepted only when all currently confirmed
  // final-frame conditions are satisfied.
  const bool healthValid = (healthRaw != 0xFFFF && healthRaw != 0x0000);
  const bool finalFrame = (status == STATUS_BODY_COMPLETE && healthValid);

  if (finalFrame && !finalMeasurementEmitted) {
    finalMeasurementEmitted = true;
    printFinalMeasurement(dev, data, weightKg, healthRaw);
  }
}

// -----------------------------------------------------------------------------
// NimBLE callbacks
// -----------------------------------------------------------------------------

class ScaleScanCallbacks : public NimBLEScanCallbacks {
 public:
  void inspect(const NimBLEAdvertisedDevice *dev) {
    if (!dev->haveManufacturerData()) return;

    const std::string mfg = dev->getManufacturerData();
    if (mfg.size() != MFG_LEN) return;

    const uint8_t *data = reinterpret_cast<const uint8_t *>(mfg.data());
    if (!isYoHealthPacket(data, mfg.size())) return;

    processYoHealth(dev, data);
  }

  void onDiscovered(const NimBLEAdvertisedDevice *dev) override {
    inspect(dev);
  }

  void onResult(const NimBLEAdvertisedDevice *dev) override {
    inspect(dev);
  }

  void onScanEnd(const NimBLEScanResults &results, int reason) override {
    Serial.printf("[BLE] Scan ended (reason=%d), restarting\n", reason);
    NimBLEDevice::getScan()->start(SCAN_PERIOD_MS, false, true);
  }
};

static ScaleScanCallbacks scanCallbacks;

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(800);

  Serial.println();
  Serial.println("Laica PS7002 / YoHealth research logger");
  Serial.println("MIT licensed research implementation");
  Serial.println();
  Serial.printf("Profile: %s, age=%u, height=%.1f cm\n",
                PROFILE_MALE ? "male" : "female",
                PROFILE_AGE_YEARS,
                PROFILE_HEIGHT_CM);
  Serial.println("Waiting for final YoHealth body-composition frame (status 0x86)...");

  NimBLEDevice::init("");
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&scanCallbacks, SCAN_DUPLICATES);
  scan->setActiveScan(ACTIVE_SCAN);
  scan->setMaxResults(0);
  scan->start(SCAN_PERIOD_MS, false, true);
}

void loop() {
  // Re-arm the session after the scale disappears from the air.
  if (sessionActive &&
      (uint32_t)(millis() - lastYoHealthSeenMs) > SESSION_GAP_MS) {
    sessionActive = false;
    finalMeasurementEmitted = false;
    Serial.println("[YoHealth] Session closed; ready for next weighing");
  }

  delay(100);
}
