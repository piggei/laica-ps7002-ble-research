# Device Compatibility

The project is named after the **Laica PS7002**, but one of its main goals is to establish how broadly the YoHealth protocol and algorithm are shared.

## Current matrix

| Device | BLE protocol | Algorithm | Evidence |
|---|---|---|---|
| Laica PS7002 Smart | CONFIRMED | CONFIRMED for current app-exposed fields | direct ESP32 captures + app comparison |
| Laica PS7200L | historically documented as YoHealth family | historical source uses same `getHealth()` routine | 2018 reverse-engineering artifacts; not independently re-tested here yet |
| Other Laica / YoHealth scales | UNKNOWN | UNKNOWN | community reports requested |

## What counts as protocol compatibility

Strong evidence includes:

- BLE local name `YoHealth`;
- 14-byte NimBLE Manufacturer Data matching `02 A1 09 FF ... AA`;
- weight at bytes 4–5;
- health/impedance at bytes 6–7;
- compatible status behavior;
- mode byte matching the historical type/precision scheme;
- checksum matching the documented sum.

## What counts as algorithm compatibility

For full compatibility, compare a complete body-composition weighing with the companion app using the same:

```text
sex
age
height
weight
impedance
```

Matching BMI, body fat, water, muscle and BMR is strong evidence that the same native algorithm family is used.

If another app/model exposes bone mass, visceral fat or body age, those fields are particularly valuable because the current PS7002 app used in this project hides some of them.

## How to report another device

Use the repository's **Device compatibility report** Issue Form and provide:

- manufacturer and exact model;
- app name/version;
- BLE local name;
- one or more raw Manufacturer Data frames;
- displayed weight;
- body-composition values when available;
- capture environment.

Do not include unrelated BLE traffic or unnecessary personal data.
