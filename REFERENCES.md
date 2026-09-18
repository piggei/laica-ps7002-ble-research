# References

The project is based on direct PS7002 captures plus independent analysis of historical YoHealth/Laica reverse-engineering artifacts.

## Public historical background

- **Reverse Engineering BLE Devices — Laica PS7200L protocol**  
  https://reverse-engineering-ble-devices.readthedocs.io/en/latest/protocol_description/00_protocol_description.html#laica-ps7200l-protocol

  The 2018 documentation established that a related Laica scale broadcasts measurements through BLE advertising, uses the name `YoHealth`, carries weight and a second health value, and relies on sex/age/height plus a native `getHealth()` routine for body-composition calculations.

- **Historical GSoC reverse-engineering repository**  
  https://gitlab.com/sergioalberti/gsoc-blereverse

  The historical archive contains the decompiled Android application, native-library decompilation and ARM disassembly used as external research references in this project.

## Historical source locations used for semantic mapping

Within the historical archive, the key classes/artifacts are:

```text
laica_PS7200L_reveng/
├── laicabodytouch_source/
│   ├── com/example/hellojni/HelloJni.java
│   ├── com/yohealth/api/btscale/YoHealthBtScaleHelper.java
│   └── com/whb/loease/activity/MainActivity.java
└── libyohealth_source/
    ├── libyohealth.so.c
    └── arm_objdump_output
```

These establish:

- `getHealth(sex, age, height, weight, impedance)` input order;
- the complete eight-field output mapping;
- mode/precision-byte handling;
- realtime/stable status logic;
- historical UI/storage use of bone mass and BMR.

The proprietary/decompiled files themselves are intentionally **not redistributed** in this MIT repository.

## Direct evidence generated in this project

Real passive BLE captures from the PS7002 established or confirmed:

- local name `YoHealth`;
- 14-byte NimBLE manufacturer-data representation;
- weight and health/impedance offsets;
- PS7002 status progression including `0x80`, `0x82` and `0x86`;
- mode byte `0x21` on the reference unit;
- checksum algorithm;
- exact algorithm agreement with current app output for the reference measurement;
- subsequent agreement for all fields exposed by the current PS7002 app on another weighing.
