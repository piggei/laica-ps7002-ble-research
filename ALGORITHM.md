# Recovered YoHealth Body-Composition Algorithm

## 1. Source and methodology

The historical YoHealth/Laica Android application calls a native function named `getHealth()` from `libyohealth.so`.

The 2018 RetDec C output contains type-recovery errors, so the reconstruction used several sources together:

1. JNI/native function signatures;
2. ARM disassembly and floating-point constants;
3. decompiled Android Java source;
4. real PS7002 BLE measurements;
5. comparison with values displayed by the current Laica app.

The Java source is particularly important because it establishes both the **input order** and the **meaning of every returned field**.

## 2. Inputs — SOURCE-MAPPED

Historical Java calls:

```text
getHealth(sex, age, height, weight, impedance)
```

with:

```text
sex          0 = male, 1 = female
age          years
height       cm
weight       kg
impedance    raw 16-bit YoHealth health value
```

Reference vector:

```text
sex          0
age          55
height       175
weight       80.7
impedance    665
```

## 3. Native output order — SOURCE-MAPPED

`getHealth()` formats eight comma-separated fields. Historical `YoHealthBtScaleHelper` maps them as follows:

| Native index | Native representation | Android interpretation |
|---:|---|---|
| 0 | float | BMI |
| 1 | fraction | body fat × 100 |
| 2 | fraction | water × 100 |
| 3 | fraction | muscle × 100 |
| 4 | native bone value | divide by 30 -> bone mass |
| 5 | fraction | visceral fat × 100 |
| 6 | integer | body age |
| 7 | integer | BMR |

This mapping definitively resolves the fields previously documented as `bone candidate`, `native metric X` and `native metric Y`.

The old Android application rounded several displayed percentages to one decimal place. Newer Laica apps may format the same underlying values differently.

## 4. BMI — CONFIRMED

```text
height_m = height_cm / 100
BMI = weight_kg / height_m²
```

Reference:

```text
80.7 / 1.75² = 26.351020...
current app = 26.35
```

## 5. Lean-mass intermediate — RECOVERED

```text
lean_mass_kg =
    0.00067 * height_cm²
  + 2.0
  + 0.53 * weight_kg
  - 0.00095 * impedance
  - 3.0 * sex
  - 0.05 * age
```

Reference:

```text
lean_mass = 61.908 kg
```

This is an internal estimate, not one of the eight user-facing YoHealth fields.

## 6. Body fat — CONFIRMED

```text
fat_fraction = (weight - lean_mass) / weight
```

The native routine applies a correction below 10%:

```text
if fat_fraction < 0.10:
    fat_fraction = fat_fraction + 0.7 * (0.10 - fat_fraction)
```

User-facing value:

```text
body_fat_pct = fat_fraction * 100
```

Reference:

```text
23.286245... %
current app = 23.28 %
```

## 7. Water — CONFIRMED

```text
water_fraction = 0.73 * lean_mass / weight
water_pct = water_fraction * 100
```

Reference:

```text
56.001041... %
current app = 56 %
```

## 8. Muscle — CONFIRMED for the male PS7002 profile

### Male

```text
muscle_pct =
    (7.78 * height_cm + 334 - 9.8 * age) / weight_kg
    + 24.4
```

Reference:

```text
38.730855... %
current app = 38.73 %
```

### Female — RECOVERED

```text
muscle_pct =
    (7.74 * height_cm - 318 - 9.8 * age) / weight_kg
    + 24.4
```

The female branch is recovered from native code but still needs direct PS7002 validation with a female app profile.

## 9. Bone mass — SOURCE-MAPPED

The native calculation first produces:

```text
bone_mass =
    0.0077200001 * weight_kg
  + 0.0045 * height_cm
  + 1.95
  - 0.00636 * age
  - 0.000232 * impedance
```

For female profiles:

```text
bone_mass *= 0.75
```

The native CSV field is:

```text
field_4 = 30 * bone_mass
```

Historical Android code explicitly divides field 4 by 30 and passes the result as `boneMass`.

Reference male result:

```text
bone_mass = 2.856424 kg
native field 4 = 85.692720
```

The current PS7002 app used in this project does not expose bone mass, so the **semantic mapping is confirmed from source**, while current-app display validation is not available.

## 10. Visceral fat — SOURCE-MAPPED

The native field is derived from body-fat fraction:

```text
male:   visceral_fraction = fat_fraction * 0.45
female: visceral_fraction = fat_fraction * 0.20
```

Historical Android code multiplies this by 100 and passes it as `visceralFatPercentage`.

Therefore:

```text
male:   visceral_fat_pct = body_fat_pct * 0.45
female: visceral_fat_pct = body_fat_pct * 0.20
```

Reference male result:

```text
visceral_fat_pct = 10.478810... %
```

The current PS7002 app used in this project does not expose this field.

## 11. BMR — CONFIRMED on current PS7002 app

The native implementation applies C `round()` before returning the integer.

### Male

```text
BMR = round(
    13.7 * weight_kg
  + 5.0 * height_cm
  - 6.8 * age
  + 66
)
```

### Female

```text
BMR = round(
    9.6 * weight_kg
  + 1.8 * height_cm
  - 4.7 * age
  + 655
)
```

Reference male prediction:

```text
1673 kcal/day
```

A subsequent PS7002 weighing matched all values exposed by the current app, including BMR.

## 12. Body age — SOURCE-MAPPED

```text
if age < 20:        body_age = age
else if BMI > 28:   body_age = age + 15
else if BMI > 26:   body_age = age + 12
else if BMI > 25:   body_age = age + 7
else if BMI > 23:   body_age = age + 4
else if age < 30:   body_age = 18
else if age <= 44:  body_age = age - 12
else:               body_age = age - 16
```

Reference:

```text
BMI 26.35, age 55 -> body age 67
```

Historical Android code passes native field 6 as `bodyAge`. The current PS7002 app used for validation does not display it.

## 13. Complete reference vector

Input:

```text
sex=male
age=55
height=175 cm
weight=80.7 kg
impedance=665
```

Calculated:

```text
BMI                  26.351020
lean mass            61.908000 kg
body fat             23.286245 %
water                56.001041 %
muscle               38.730855 %
bone mass             2.856424 kg
visceral fat         10.478810 %
body age             67
BMR                 1673 kcal/day
```

## 14. Display formatting is not the algorithm

The historical Android application rounded several values to one decimal place using `BigDecimal` with half-up rounding. The current PS7002 application observed during this project displays some values with different precision, for example body fat and BMI with two decimals.

The firmware and Python reference calculator therefore preserve more numerical precision and do not try to imitate a specific app version's presentation layer.

## 15. Remaining algorithm-validation work

The complete field semantics are now known, but useful validation remains:

- female-profile measurements;
- larger sample counts across different weight/impedance values;
- confirmation on other Laica/YoHealth models;
- comparison of hidden historical fields where another app/model exposes them.
