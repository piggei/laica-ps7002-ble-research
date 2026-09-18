# Contributing

Contributions are welcome, especially reproducible measurements, compatible-device reports, protocol observations, bug reports, and corrections to the recovered algorithm documentation. The PS7002 is the reference device, but a major goal is to determine which other LAICA/YoHealth scales share the same protocol and calculation algorithm.

## Use the structured issue forms

GitHub provides three project-specific issue forms:

- **Sample measurement / algorithm validation** — one weighing, including the exact scale model, with raw YoHealth data and the values shown by the companion app.
- **Device compatibility report** — another scale/model that appears to use the same or a related protocol.
- **Bug report** — a reproducible problem in the firmware, parser, calculator, or documentation.

Blank public issues are disabled so reports stay structured and comparable.

## Measurement privacy

A useful algorithm-validation sample normally needs only:

- scale model;
- profile sex setting used by the app, if the contributor chooses to share it;
- age in whole years, if shared;
- height in cm, if shared;
- weight;
- raw health/impedance value;
- final Manufacturer Data frame;
- body-composition values shown by the app.

Do **not** publish names, exact dates of birth, account identifiers, addresses, medical records, or other information that is not required to reproduce the calculation.

One complete weighing per issue is preferred because it makes comparison and discussion easier.

## Protocol observations

When reporting raw BLE data:

- remove unrelated nearby BLE traffic;
- include the exact scale model and BLE local name when known;
- include the displayed weight corresponding to the frame when possible;
- preserve byte order exactly;
- distinguish observation from interpretation.

## Code contributions

Keep recovered facts, experimentally confirmed behavior, and hypotheses clearly separated in comments and documentation. Do not add proprietary binaries or decompiled proprietary source code to this repository.

Original contributions to this repository are made under the MIT License unless explicitly stated otherwise.
