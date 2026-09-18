# Contributing

Contributions are welcome, especially reproducible measurements, compatible-device reports, protocol observations, bug reports and documentation corrections.

The PS7002 is the reference device, but a major goal is to determine which other Laica/YoHealth scales share the same protocol and body-composition algorithm.

## Structured issue forms

Use one of the included GitHub Issue Forms:

- **Sample measurement / algorithm validation** — one complete weighing with the exact scale model, protocol data and app comparison;
- **Device compatibility report** — another model that appears to use the same or a related YoHealth protocol;
- **Bug report** — a reproducible problem in firmware, calculator or documentation.

## Measurement privacy

A useful validation sample normally needs only:

- scale model;
- app/version;
- profile sex setting, age in whole years and height if the contributor chooses to share them;
- weight;
- raw impedance/health;
- final Manufacturer Data;
- calculated fields;
- values shown by the app.

Do **not** publish names, exact dates of birth, account identifiers, addresses, medical records or unrelated health information.

## Hidden fields are useful

Historical YoHealth source identifies bone mass, visceral fat and body age even though some current Laica app versions may not show them. If another model/app does display those fields, please report them: they are especially valuable for cross-device validation.

## Protocol observations

When posting BLE data:

- remove unrelated nearby BLE traffic;
- preserve byte order exactly;
- include the exact model and BLE local name;
- include the displayed weight associated with the frame;
- report status and mode bytes when possible;
- distinguish observed bytes from interpretations.

## Code contributions

Keep these categories separate in comments/docs:

- **CONFIRMED** direct PS7002/app behavior;
- **SOURCE-MAPPED** meanings established by historical app source;
- **RECOVERED** numerical behavior reconstructed from native code;
- **OPEN** protocol or compatibility questions.

Do not add proprietary binaries or decompiled proprietary source code to this repository.

Original contributions are made under the MIT License unless explicitly stated otherwise.
