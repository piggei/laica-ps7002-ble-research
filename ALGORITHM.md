# Recovered YoHealth Body-Composition Algorithm

## 1. Source and methodology

A historical ARM native library used by the YoHealth/Laica application exposed a function named `getHealth()`. The available 2018 RetDec decompilation was incomplete and sometimes assigned incorrect C types to ARM register values, but the numerical operations and literal constants were still recoverable.

We reconstructed the calculation by:

1. locating JNI `Java_com_example_hellojni_HelloJni_getHealth`;
2. following the ARM call into native `getHealth()`;
3. mapping stack/register arguments back to sex, age, height, weight and health/impedance;
4. decoding floating-point literal pools from ARM disassembly;
5. rewriting the arithmetic in ordinary C/Python;
6. checking the result against a real PS7002 measurement and Laica app output.

The exact match of four independently displayed app values provides strong validation of the interpretation.

## 2. Inputs

The recovered algorithm uses:

```text
sex          0 = male, 1 = female
age          years
height       cm
weight       kg
impedance    raw 16-bit YoHealth health value
```

For our reference measurement:

```text
sex          0
age          55
height       175
weight       80.7
impedance    665
```

## 3. BMI — CONFIRMED

```text
height_m = height_cm / 100
BMI = weight_kg / height_m²
```

Reference:

```text
80.7 / 1.75² = 26.351020...
app = 26.35
```

## 4. Lean-mass intermediate — RECOVERED

The central internal estimate is:

```text
lean_mass_kg =
    0.00067 * height_cm²
  + 2.0
  + 0.53 * weight_kg
  - 0.00095 * impedance
  - 3.0 * sex
  - 0.05 * age
```

Reference result:

```text
lean_mass = 61.908 kg
```

This internal value is not necessarily shown by the Laica app.

## 5. Body fat — CONFIRMED

Initial fraction:

```text
fat = (weight - lean_mass) / weight
```

The native routine applies a special correction if the result is below 10%:

```text
if fat < 0.10:
    fat = fat + 0.7 * (0.10 - fat)
```

Displayed percentage:

```text
body_fat_pct = fat * 100
```

Reference:

```text
23.286245... %
app = 23.28 %
```

## 6. Water — CONFIRMED

```text
water_fraction = 0.73 * lean_mass / weight
water_pct = water_fraction * 100
```

Reference:

```text
56.001041... %
app = 56 %
```

The factor 0.73 explains the empirical relationship first observed before the native routine was fully reconstructed.

## 7. Muscle — CONFIRMED for male reference profile

### Male

```text
muscle_pct =
    (7.78 * height_cm + 334 - 9.8 * age) / weight_kg
    + 24.4
```

Reference:

```text
38.730855... %
app = 38.73 %
```

### Female

```text
muscle_pct =
    (7.74 * height_cm - 318 - 9.8 * age) / weight_kg
    + 24.4
```

The female branch is directly recovered but has not yet been validated with a PS7002 female-profile capture.

## 8. BMR — RECOVERED, validation pending

The routine uses sex-specific equations and rounds the result.

### Male

```text
BMR = round(
    13.7 * weight_kg
  + 5.0 * height_cm
  - 6.8 * age
  + 66
)
```

Reference prediction:

```text
1673 kcal/day
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

## 9. Body age — RECOVERED

The native routine implements a simple BMI/age heuristic:

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

Reference prediction:

```text
BMI 26.35, age 55 -> body age 67
```

Whether the current Laica UI exposes this field remains to be checked.

## 10. Bone candidate — HYPOTHESIS

The native routine contains the following sex-dependent term:

```text
base =
    0.0077200001 * weight_kg
  + 0.0045 * height_cm
  + 1.95
  - 0.00636 * age
  - 0.000232 * impedance

if female:
    base *= 0.75
```

For the reference male measurement:

```text
base = 2.856424
```

This is a biologically plausible bone-mass value in kg and is therefore logged as:

```text
bone_candidate_kg = 2.856
```

However, caution is required: the native function subsequently transforms this intermediate before formatting one of its return fields. Until the Laica app's bone value is compared against several measurements, the semantic mapping remains a **hypothesis**, not a confirmed result.

## 11. Additional native metrics — semantics unknown

Two further expressions are visible in the native routine:

```text
native_metric_x = 30 * bone_candidate
```

and:

```text
native_metric_y = fat_fraction * coefficient
coefficient = 0.45 male
coefficient = 0.20 female
```

Their numerical formulas are recovered, but their user-facing labels are not yet proven. They are emitted by the research logger under neutral names rather than being prematurely called visceral fat, score, protein, etc.

## 12. Reference vector

Input:

```text
sex=male
age=55
height=175 cm
weight=80.7 kg
impedance=665
```

Predicted:

```text
BMI                  26.351020
lean mass            61.908000 kg
body fat             23.286245 %
water                56.001041 %
muscle               38.730855 %
bone candidate        2.856424 kg
BMR                 1673 kcal/day
body age              67
native metric X       85.692720
native metric Y        0.104788
```

## 13. Confidence policy

The project intentionally distinguishes formula recovery from semantic naming.

A formula may be numerically present in the binary yet still have an uncertain UI meaning. New measurements should therefore be used both to test numeric accuracy and to identify which native output corresponds to each current app field.
