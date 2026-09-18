# Validation Plan

The project now has two distinct validation goals:

1. verify the recovered YoHealth calculations across more real PS7002 measurements;
2. determine which other Laica/YoHealth scales share the same BLE protocol and algorithm.

## 1. Keep profile variables identical

The ESP32/Python profile must match the companion app:

```text
sex
age
height
```

Initial reference profile:

```text
sex       male
age       55
height    175 cm
```

Age directly changes several formulas.

## 2. Perform a complete measurement

For body-composition data, use valid electrode contact and wait for the complete measurement.

On the PS7002 the final frame is normally:

```text
status = 0x86
health/impedance = numeric
mode = 0x21
```

The firmware now follows the more general historical YoHealth stable-state logic rather than accepting only one hard-coded status value.

## 3. Record ESP32 results

Record:

- final raw Manufacturer Data;
- status byte;
- device mode byte/code;
- weight;
- impedance;
- BMI;
- body fat;
- water;
- muscle;
- bone mass;
- visceral fat;
- body age;
- BMR.

## 4. Record app results

Record every value the current companion app actually displays.

A field may legitimately be marked **not displayed**. The current PS7002 app used during this project does not expose every historical YoHealth output field.

Useful fields include:

- weight;
- BMI;
- body fat %;
- water %;
- muscle %;
- BMR;
- bone mass, if shown;
- visceral fat, if shown;
- body age, if shown.

## 5. Use the CSV template

Copy `data/measurements-template.csv` and append one row per weighing.

A good dataset includes:

- repeated measurements around similar weights;
- naturally different weights and impedance values;
- different profile values where intentionally tested;
- measurements from additional scale models.

## 6. Matching rules

Do not confuse display formatting with formula differences.

For example, the reference algorithm yields:

```text
body fat = 23.286245... %
```

while one current app displays:

```text
23.28 %
```

The historical Bodytouch app rounded many metrics to one decimal place, so different app versions can display the same underlying calculation differently.

## 7. Reference vector

```text
Model      LAICA PS7002
Sex        male
Age        55
Height     175 cm
Weight     80.7 kg
Impedance  665
```

Expected recovered values:

```text
BMI                  26.351020
Body fat             23.286245 %
Water                56.001041 %
Muscle               38.730855 %
Bone mass             2.856424 kg
Visceral fat         10.478810 %
Body age             67
BMR                 1673 kcal/day
```

The first four displayed composition values matched the current app exactly to its displayed precision, and a later PS7002 weighing was reported to match every field exposed by the current app.

## 8. Useful protocol tests

Optional tests that help characterize the wire protocol:

- stand on the scale without valid electrode contact;
- capture the transition from realtime to stable weight;
- change weight units if supported;
- repeat identical measurements in separate sessions;
- test a model using mode precision digit `2` if one is found;
- capture status/error conditions.

## 9. Suggested report format

```text
Scale model: LAICA PS7002
App/version: ...
Profile: male, 55, 175 cm

ESP32:
weight=...
impedance=...
status=...
mode=...
bmi=...
fat=...
water=...
muscle=...
bone=...
visceral_fat=...
body_age=...
bmr=...

App:
weight=...
bmi=...
fat=...
water=...
muscle=...
bone=not displayed
visceral_fat=not displayed
body_age=not displayed
bmr=...

Raw MFG: ...
```

## 10. GitHub validation issues

Use **Sample measurement / algorithm validation** for a complete weighing.

Use **Device compatibility report** for a new model or protocol variant.

One weighing per issue is preferred. Do not publish names, exact dates of birth or unrelated personal/medical information; age in whole years is sufficient.
