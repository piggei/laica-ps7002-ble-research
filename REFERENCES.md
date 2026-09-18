# References

The project is based primarily on direct PS7002 captures and independent analysis of historical native-code artifacts supplied to the research effort.

Public historical background:

- **Reverse Engineering BLE Devices — Laica PS7200L protocol**  
  https://reverse-engineering-ble-devices.readthedocs.io/en/latest/protocol_description/00_protocol_description.html#laica-ps7200l-protocol

  This 2018 documentation established several important facts for a related Laica scale: measurements are broadcast through advertising without pairing; weight and a second "health" value are present in manufacturer-specific data; gender, age and height are used by the app to derive body-composition values; and the calculation eventually reaches a proprietary `libyohealth.so` `getHealth()` routine.

- **Historical decompilation location referenced by the 2018 documentation**  
  https://gitlab.com/sergioalberti/gsoc-blereverse/tree/master/laica_PS7200L_reveng/libyohealth_source

The original/decompiled proprietary library material is intentionally **not redistributed** in this MIT repository. See `NOTICE.md`.

## Direct evidence generated in this project

The current PS7002 protocol notes additionally come from real passive BLE captures made with an ESP32. These established:

- local name `YoHealth`;
- PS7002 manufacturer-data framing;
- actual 14-byte NimBLE representation including company-ID bytes;
- weight byte offsets and endian order;
- final `0x86` body-composition frame;
- numeric health/impedance input;
- checksum algorithm;
- exact agreement of recovered BMI, body-fat, water and muscle calculations with the current Laica app for the initial reference measurement.
