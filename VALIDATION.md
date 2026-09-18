# Validation Plan

The goal is to determine whether the recovered YoHealth calculations reproduce the current Laica PS7002 app across multiple real measurements, not only one reference point.

## 1. Keep profile variables identical

Before every validation series, ensure the ESP32 constants exactly match the app profile:

```text
sex
age
height
```

For the initial campaign:

```text
sex       male
DOB       1970-12-10
age       55
height    175 cm
```

Age changes the formulas directly. Update the sketch when the app profile age changes.

## 2. Perform a complete measurement

For body-composition data, use the scale as intended for BIA measurement (including electrode contact). Wait until the scale/app completes the measurement.

The ESP32 should eventually report:

```text
Status : 0x86 (BODY_COMPLETE)
```

with a numeric impedance value.

## 3. Record both sides

From the ESP32 record:

- final raw manufacturer data;
- weight;
- impedance;
- BMI;
- body fat;
- water;
- muscle;
- BMR;
- body age;
- bone candidate;
- native metric X/Y.

From the Laica app record every value the UI displays, especially:

- weight;
- BMI;
- body fat %;
- water %;
- muscle %;
- bone mass;
- BMR;
- body/metabolic age if present.

## 4. Use the CSV template

Copy `data/measurements-template.csv` and append one row per weighing.

A useful dataset should include:

- repeated measurements around the same weight;
- naturally different weights over time;
- different impedance values;
- ideally 10+ complete body-composition samples.

The most important current target is confirming the semantic mapping of the bone candidate and the two unidentified native metrics.

## 5. What counts as a match

Consider the app's display rounding. For example, the recovered reference body-fat value is:

```text
23.286245 %
```

while the app displays:

```text
23.28 %
```

Do not infer a formula discrepancy from simple formatting/rounding until the display behavior is characterized.

## 6. Useful negative tests

Optional protocol tests:

- stand on the scale without valid electrode contact;
- change unit mode if supported;
- repeat exactly the same measurement after the first session closes;
- observe whether `0x82` always precedes `0x86`;
- note packets when only weight is measured.

## 7. Report format

When returning results for analysis, a compact report like this is ideal:

```text
Profile: male, 55, 175 cm

ESP32:
weight=...
impedance=...
bmi=...
fat=...
water=...
muscle=...
bone_candidate=...
bmr=...
body_age=...
native_x=...
native_y=...

App:
weight=...
bmi=...
fat=...
water=...
muscle=...
bone=...
bmr=...
body_age=...

Raw MFG: ...
```

A CSV row from the logger plus the app values is even better.
