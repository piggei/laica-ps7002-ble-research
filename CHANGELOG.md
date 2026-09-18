# Changelog

## 0.2.0-research - 2026-09-18

Major reverse-engineering milestone.

- Recovered the complete semantic mapping of all eight historical YoHealth `getHealth()` output fields from the decompiled Android consumer.
- Replaced `bone candidate` / `native metric X` / `native metric Y` with their source-mapped meanings:
  - bone mass;
  - visceral fat percentage;
  - body age.
- Confirmed the `getHealth()` input order as sex, age, height, weight and impedance.
- Documented that current PS7002 app versions may hide historical YoHealth fields even though the algorithm still produces them.
- Marked BMR as confirmed against a subsequent real PS7002 weighing in which all current-app-exposed metrics matched the research implementation.
- Identified Manufacturer Data byte 11 as the historical YoHealth device type / weight-precision code; `0x21` means body-composition device with 0.1 kg resolution.
- Improved status decoding using the historical app's stable/lock bit and no-BIA state logic instead of relying only on a hard-coded `0x86` value.
- Updated the firmware to calculate and print the complete named YoHealth result set.
- Updated CSV output and validation schema with status, device mode, bone mass and visceral fat.
- Updated Issue Forms for complete cross-device and hidden-field validation.
- Added `REVERSE_ENGINEERING.md` and `COMPATIBILITY.md`.
- Expanded protocol and algorithm documentation with source-mapped findings.

## 0.1.2-research - 2026-09-18

- Added the PS7002 product image directly below the README title.
- Added an explicit compatibility-scope section: PS7002 is the directly validated reference device, while other LAICA/YoHealth models are actively sought for confirmation.
- Clarified that cross-model compatibility is a research target, not an assumption.
- Made the sample-measurement Issue Form explicitly multi-model and kept the exact scale model as a required field.
- Added scale model and BLE local-name columns to the validation CSV schema.
- Updated contribution and validation guidance for the cross-device campaign.
- Clarified that the supplied product image is not automatically covered by the MIT License.

## 0.1.1-research - 2026-09-18

- Added structured GitHub Issue Forms for sample measurements, compatible-device reports, and bugs.
- Added issue-template chooser configuration with blank public issues disabled.
- Added CONTRIBUTING.md with reporting and privacy guidance.
- Documented the GitHub validation workflow in README.md and VALIDATION.md.

## 0.1.0-research - 2026-09-18

Initial research repository.

- Isolated PS7002 `YoHealth` BLE advertising.
- Confirmed passive, connectionless measurement flow.
- Documented 14-byte NimBLE manufacturer-data frame.
- Confirmed big-endian weight encoding.
- Confirmed checksum algorithm.
- Identified `0x86` final body-composition frame with numeric health/impedance.
- Reconstructed core `getHealth()` formulas from historical native code.
- Matched BMI, body-fat %, water % and muscle % against a real app measurement.
- Added research ESP32 logger.
- Added independent Python reference calculator.
- Added validation CSV schema and experiment protocol.
- Licensed original project code/documentation under MIT.
