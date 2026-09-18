# PS7002 / YoHealth BLE Protocol

This document records the current reverse-engineered BLE protocol of the Laica PS7002 Smart scale.

## 1. Discovery history

The initial assumption was that the PS7002 might require a GATT connection. Real-world observation showed otherwise: the scale remains silent before a completed weighing and then broadcasts the result, without pairing or a client request.

A historical reverse-engineering project for the related Laica PS7200L described a device named `YoHealth` with measurement data in BLE manufacturer-specific advertising. That gave us a strong candidate protocol.

A passive ESP32 scan of the PS7002 then identified the same local name:

```text
NAME: YoHealth
```

and manufacturer-specific data matching the same 12-byte YoHealth payload family, preceded by the two BLE company-ID bytes returned by NimBLE.

## 2. Manufacturer data layout

NimBLE-Arduino exposes the complete manufacturer-data field, including the two-byte BLE Company Identifier.

Observed PS7002 frame:

```text
02 A1 09 FF WW WW ZZ ZZ SS FF FF 21 CC AA
```

| Offset | Size | Example | Meaning | Confidence |
|---:|---:|---|---|---|
| 0 | 2 | `02 A1` | company identifier bytes as exposed by NimBLE | CONFIRMED |
| 2 | 2 | `09 FF` | YoHealth protocol header | CONFIRMED |
| 4 | 2 | `03 27` | weight × 10, big-endian | CONFIRMED |
| 6 | 2 | `02 99` | raw health / bio-impedance value, big-endian | CONFIRMED as algorithm input; physical ohm interpretation highly plausible |
| 8 | 1 | `86` | measurement status / flags | CONFIRMED byte, semantics partially inferred |
| 9 | 2 | `FF FF` | unknown / reserved | UNKNOWN |
| 11 | 1 | `21` | constant in all captures so far | OBSERVED |
| 12 | 1 | `15` | checksum | CONFIRMED |
| 13 | 1 | `AA` | terminator | CONFIRMED |

## 3. Weight encoding

Weight is the unsigned big-endian integer at manufacturer-data bytes 4–5 divided by 10:

```text
03 27 = 0x0327 = 807
807 / 10 = 80.7 kg
```

Formula:

```text
weight_kg = BE16(mfg[4], mfg[5]) / 10
```

## 4. Health / impedance encoding

The two bytes at offsets 6–7 are unavailable during the weight-only phase:

```text
FF FF
```

and become a numeric value after a completed body-composition measurement:

```text
02 99 = 665
```

The recovered YoHealth algorithm consumes this value directly in the body-composition equations. It behaves exactly like an impedance term and values such as 665 are physically plausible for foot-to-foot BIA, so the project labels it `impedance` in code while retaining the historical term `health` in documentation where useful.

## 5. Status byte

Statuses observed so far:

| Status | Observation | Current interpretation |
|---|---|---|
| `0x80` | weight changing / health `FFFF` | measurement in progress / no impedance |
| `0x82` | stable weight observed in earlier capture | stable weight candidate |
| `0x86` | stable weight + numeric health value | final body-composition measurement |

Only `0x86` is currently used to emit a final body-composition result.

The exact bit-level meaning is not yet proven. Future captures should test whether these are independent flags or enumerated states.

## 6. Checksum

The checksum is the low eight bits of the sum of bytes 0 through 11 of the manufacturer-data block:

```text
checksum = sum(mfg[0:12]) & 0xFF
```

For the final reference packet:

```text
02 A1 09 FF 03 27 02 99 86 FF FF 21 15 AA
                                    ^^
```

summing bytes `0..11` modulo 256 gives `0x15`, exactly matching byte 12.

This also matches historical YoHealth/PS7200L sample frames when the two company-ID bytes are included.

## 7. Complete reference measurement

Reference profile:

```text
sex        male
age        55
height     175 cm
```

Final packet:

```text
02 A1 09 FF 03 27 02 99 86 FF FF 21 15 AA
```

Decoded radio values:

```text
weight       80.7 kg
health/Z     665
status       0x86
checksum     valid
```

Laica app values supplied for the same measurement:

```text
body fat     23.28 %
water        56 %
BMI          26.35
muscle       38.73 %
```

The independently reconstructed calculations produce the same values to the displayed precision; see `ALGORITHM.md`.

## 8. Device addressing

A captured PS7002 advertised with:

```text
MAC ff:ff:ff:ff:ff:d0
NAME YoHealth
```

The implementation deliberately does **not** depend on this MAC address. Device identification is based on protocol framing so the decoder has a better chance of working across PS7002 units and related YoHealth-based scales.

## 9. Passive operation

No GATT connection, pairing or command transmission is required for the observed measurement flow. The ESP32 implementation is receive-only.

This makes the protocol suitable for a future Passive BLE Monitor decoder.

## 10. Open protocol questions

Still to be established with additional captures:

- exact bit meaning of status byte `0x80/0x82/0x86`;
- semantic meaning of bytes 9–10;
- whether byte 11 is always `0x21` across units/modes;
- unit-mode behavior (kg/lb/st);
- behavior on weight-only measurements without electrode contact;
- whether related Laica models share the exact same manufacturer identifier and final-state rules.
