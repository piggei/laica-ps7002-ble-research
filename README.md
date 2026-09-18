# Laica PS7002 BLE Research

Independent reverse-engineering and interoperability research for the **Laica PS7002 Smart** body-composition scale.

The project passively receives the scale's BLE advertising packets, identifies the final body-composition result, decodes weight and the raw YoHealth "health" value, and reproduces the body-composition calculations recovered from the historical YoHealth native library.

> **Status:** research / validation. Weight, BLE framing, checksum, BMI, body-fat %, water % and muscle % are strongly confirmed. BMR and body-age formulas are recovered from the native routine and need broad multi-sample validation. The proposed bone-mass mapping is explicitly experimental.

## Why this project exists

The PS7002 broadcasts its measurement without pairing. That makes it a good candidate for a fully passive Home Assistant integration, including a future decoder for Passive BLE Monitor or a similar BLE gateway.

A historical reverse-engineering effort on a related Laica/YoHealth scale showed that the vendor application used a native `libyohealth.so` routine named `getHealth()`. In 2026 we re-examined the decompiled C and ARM disassembly, then checked the recovered formulas against real PS7002 packets and values displayed by the current Laica app.

For the reference measurement:

- male
- age 55
- height 175 cm
- weight 80.7 kg
- raw health/impedance 665

we obtain:

| Metric | Recovered calculation | Laica app |
|---|---:|---:|
| BMI | 26.3510 | 26.35 |
| Body fat | 23.2862 % | 23.28 % |
| Water | 56.0010 % | 56 % |
| Muscle | 38.7309 % | 38.73 % |

This exact agreement is the central validation result of the project so far.

## Repository layout

```text
.
├── README.md
├── PROTOCOL.md
├── ALGORITHM.md
├── VALIDATION.md
├── REFERENCES.md
├── LICENSE
├── .gitignore
├── firmware/
│   └── Laica_PS7002_Research/
│       └── Laica_PS7002_Research.ino
├── tools/
│   └── yohealth_calc.py
└── data/
    ├── README.md
    └── measurements-template.csv
```

## Hardware and software

- ESP32 with BLE support
- Arduino IDE or compatible build environment
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino), 2.x API
- Serial monitor at **115200 baud**

## Quick start

1. Install `NimBLE-Arduino` in the Arduino Library Manager.
2. Open `firmware/Laica_PS7002_Research/Laica_PS7002_Research.ino`.
3. Set the profile constants so they exactly match the Laica app:

```cpp
static constexpr bool    PROFILE_MALE      = true;
static constexpr uint8_t PROFILE_AGE_YEARS = 55;
static constexpr float   PROFILE_HEIGHT_CM = 175.0f;
```

4. Flash the ESP32 and open Serial Monitor at 115200 baud.
5. Perform a complete body-composition measurement barefoot.
6. Wait for a block beginning with:

```text
FINAL MEASUREMENT #...
```

The firmware ignores unrelated BLE traffic and accepts only validated YoHealth frames.

## What the firmware waits for

The final frame currently recognized is:

- valid YoHealth manufacturer-data framing;
- valid checksum;
- status byte `0x86`;
- health/impedance field different from `0xFFFF` and `0x0000`.

A single final result is emitted for each measurement session. The logger re-arms after a configurable period of radio silence so an identical result can still be captured during a later weighing.

See [PROTOCOL.md](PROTOCOL.md) for packet details.

## Offline calculator

The same recovered calculations are implemented independently in Python:

```bash
python3 tools/yohealth_calc.py \
  --weight 80.7 \
  --impedance 665 \
  --height 175 \
  --age 55 \
  --male
```

This provides an independent reference for regression checks and makes it easier to analyze reports without reflashing the ESP32.

## Validation campaign

Use `data/measurements-template.csv` to compare ESP32 predictions with the Laica app across many measurements. The most useful fields to report are:

- weight;
- raw impedance;
- BMI;
- body fat %;
- water %;
- muscle %;
- bone mass;
- BMR;
- body age, if displayed by the app.

See [VALIDATION.md](VALIDATION.md) for the experimental procedure and [REFERENCES.md](REFERENCES.md) for historical/public sources.

## Documentation confidence levels

Throughout the documentation:

- **CONFIRMED** means directly supported by PS7002 captures and/or exact app agreement.
- **RECOVERED** means directly visible in the historical native algorithm but not yet broadly validated on PS7002 measurements.
- **HYPOTHESIS** means a plausible semantic mapping still requiring experimental confirmation.

This distinction is intentional. Reverse engineering is much more useful when observations and interpretations are not mixed together.

## Provenance and licensing

This repository contains original interoperability code and documentation released under the **MIT License**.

The historical proprietary/decompiled `libyohealth.so` material is **not included** and is **not relicensed** by this project. It was used as an external reverse-engineering reference to understand the wire protocol and reproduce interoperable calculations. Only independently written source code, formulas expressed as facts of interoperability, experimental data schemas and original documentation belong to this repository.

The project is not affiliated with, endorsed by, or supported by Laica or the original YoHealth software authors.

## Safety / intended use

Body-composition values from consumer BIA scales are estimates. This project is intended for interoperability, personal automation and protocol research; it is not a medical device and should not be used for diagnosis or clinical decision-making.

## License

MIT. See [LICENSE](LICENSE).
