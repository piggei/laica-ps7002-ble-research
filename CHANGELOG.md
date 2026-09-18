# Changelog

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
