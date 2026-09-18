# Reverse-Engineering Notes

## Purpose

This file documents how the project moved from raw BLE captures to a complete semantic mapping of the historical YoHealth algorithm, without redistributing proprietary source or binaries.

## 1. Historical material

The research archive contains artifacts from an earlier reverse-engineering effort on a Laica PS7200L / YoHealth application, including:

- decompiled Android Java source;
- a RetDec C reconstruction of `libyohealth.so`;
- ARM disassembly of the same native library.

Those proprietary/decompiled artifacts are used as external research references only and are **not included in this MIT repository**.

## 2. Native function signature

The decompiled Android Java class declares:

```text
getHealth(int, int, int, double, int)
```

and the BLE helper calls it in this logical order:

```text
getHealth(sex, age, height_cm, weight_kg, impedance)
```

This resolved ambiguities introduced by the native ARM ABI and RetDec's imperfect type recovery.

## 3. BLE helper mapping

The historical `YoHealthBtScaleHelper` performs the following operations:

1. accepts devices named `YoHealth`;
2. extracts a 14-byte protocol region;
3. reads mode byte 11;
4. decodes raw weight from bytes 4–5;
5. passes bytes 6–7 to `getHealth()` as the final integer argument;
6. reads the status byte at offset 8;
7. distinguishes realtime and stable data;
8. maps the eight native CSV values to body-composition fields.

## 4. Complete getHealth() field mapping

The native routine returns eight comma-separated values. The Android BLE helper interprets them as:

```text
field 0 -> BMI
field 1 -> body fat fraction      -> * 100
field 2 -> water fraction         -> * 100
field 3 -> muscle fraction        -> * 100
field 4 -> bone native value      -> / 30
field 5 -> visceral fat fraction  -> * 100
field 6 -> body age integer
field 7 -> BMR integer
```

The listener interface then names the corresponding parameters:

```text
weight
bmi
fatPercentage
musclePercentage
waterPercentage
boneMass
visceralFatPercentage
bodyAge
BMR
```

This is the evidence that resolved the earlier unknown `native metric X` and `native metric Y` fields.

## 5. UI behavior in the historical app

The historical Laica Bodytouch UI displays and stores:

- weight;
- BMI;
- body fat;
- water;
- muscle;
- bone mass;
- BMR.

The listener receives visceral fat and body age as well, but the reviewed main screen does not expose them.

The current PS7002 app used for validation differs again: it does not expose every historical YoHealth field. This is why algorithm output and current UI visibility must be documented separately.

## 6. Mode byte discovery

The historical app parses byte 11 as a decimal code derived from its two hexadecimal text digits.

`0x21` therefore becomes code `21`:

```text
2 -> body-composition device family
1 -> one decimal place / divide raw weight by 10
```

It also supports an ending digit `2`, corresponding to raw weight divided by 100 and a high-precision flag.

## 7. Status semantics

The historical app checks status bit 1 as the stable-data lock bit.

```text
bit 1 clear -> realtime callback
bit 1 set   -> stable callback
```

For a body-composition scale, stable low-nibble `0x2` is treated as a weight-only/no-electrode condition, while other stable states produce the full body-composition result.

This explains the PS7002 sequence observed experimentally:

```text
0x80 -> realtime
0x82 -> stable weight, no body composition
0x86 -> stable body-composition data
```

## 8. Algorithm reconstruction

The numerical formulas were reconstructed from native operations and constants, then tested against real PS7002 measurements.

The reference measurement reproduced current-app BMI, body fat, water and muscle to display precision. A later weighing matched all values exposed by the current app, including BMR.

See `ALGORITHM.md` for equations.

## 9. Why source mapping matters

Before examining the Java consumer, several native expressions were numerically recoverable but semantically ambiguous. The Android field mapping demonstrates an important reverse-engineering principle:

> Recovering a formula and identifying what the product calls that formula are separate tasks.

The project therefore distinguishes:

- **RECOVERED** numerical behavior;
- **SOURCE-MAPPED** semantic field names;
- **CONFIRMED** results checked on the reference hardware/app.
