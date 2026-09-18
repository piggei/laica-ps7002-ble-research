# PS7002 / YoHealth BLE Protocol

This document records the reverse-engineered BLE advertising protocol used by the Laica PS7002 Smart reference device and compares it with the historical YoHealth Android implementation.

## 1. Discovery history

Real-world observation showed that the PS7002 does not require pairing or a GATT request for the measured data. It broadcasts the result in BLE advertising.

Historical reverse engineering of the related Laica PS7200L identified a device named `YoHealth` with measurement data inside Manufacturer Specific Data. Passive ESP32 captures of the PS7002 showed the same local name and compatible frame structure.

## 2. Manufacturer-data layout

NimBLE-Arduino exposes the complete manufacturer-data field including its first two manufacturer/company-ID bytes.

```text
02 A1 09 FF WW WW ZZ ZZ SS FF FF MM CC AA
```

| Offset | Size | Example | Meaning | Status |
|---:|---:|---|---|---|
| 0 | 2 | `02 A1` | manufacturer/company-ID bytes as exposed by NimBLE | CONFIRMED framing |
| 2 | 2 | `09 FF` | YoHealth protocol header | CONFIRMED |
| 4 | 2 | `03 27` | raw weight, big-endian | CONFIRMED |
| 6 | 2 | `02 99` | health / impedance input, big-endian | CONFIRMED algorithm input |
| 8 | 1 | `86` | state/flags | SOURCE-MAPPED + observed |
| 9 | 2 | `FF FF` | unknown/reserved | OPEN |
| 11 | 1 | `21` | device type / precision code | SOURCE-MAPPED |
| 12 | 1 | `15` | checksum | CONFIRMED |
| 13 | 1 | `AA` | terminator | CONFIRMED |

## 3. Device mode / precision byte

Historical `YoHealthBtScaleHelper` reads byte 11 as the two-character hexadecimal text and parses that text as a decimal integer. For digit-only values this behaves like a BCD code.

Example:

```text
raw byte 0x21 -> text "21" -> mode code 21
```

The app interprets the code as:

```text
ones digit 1 -> weight = raw / 10
ones digit 2 -> weight = raw / 100, high-precision mode

10 < mode < 20 -> weight-only device
20 < mode < 30 -> body-composition device
```

Therefore the PS7002's observed `0x21` means:

```text
body-composition device
0.1 kg weight resolution
```

This field was previously documented merely as an observed constant; the historical Android source resolves its purpose.

## 4. Weight encoding

For mode `0x21`:

```text
03 27 = 0x0327 = 807
807 / 10 = 80.7 kg
```

For a mode whose ones digit is `2`, the historical app instead divides the raw value by 100.

The firmware now implements both paths.

## 5. Health / impedance

Offsets 6–7 are `FF FF` while body-composition data is unavailable and become numeric after a valid electrode/BIA measurement.

Reference:

```text
02 99 = 665
```

The historical Android app passes this integer directly to `getHealth()` as its fifth argument. Its role in the recovered equations is exactly that of an impedance term. The code uses the name `impedance`, while documentation sometimes retains the historical name `health`.

## 6. Status byte and measurement lifecycle

Historical Android source reveals more than the three observed PS7002 byte values.

### Stable/lock bit

The app tests bit 1 (`0x02`):

```text
bit 1 clear -> realtime data
bit 1 set   -> stable/locked data
```

### Body-composition availability

For a stable body-composition device, the app treats a low nibble equal to `0x2` as a no-body-composition condition (historically named `State_WearOutShoe`). Otherwise it calculates and publishes the body-composition fields.

### PS7002 observations

| Status | Bit-level observation | Interpretation |
|---|---|---|
| `0x80` | stable bit clear | realtime / measurement in progress |
| `0x82` | stable bit set, low nibble `2` | stable weight only / no BIA result |
| `0x86` | stable bit set, low nibble `6` | stable body-composition result |

The reference firmware therefore accepts a final body-composition measurement when:

```text
checksum valid
health/impedance is numeric
status bit 1 is set
(status & 0x0F) != 0x02
```

On the PS7002 this corresponds to the observed `0x86` frame, while preserving compatibility with possible related YoHealth variants.

The historical code also uses bit 0 as an overweight-state indication. A low-voltage state constant exists in the API, but its exact raw bit mapping was not identified in the reviewed path.

## 7. Checksum

The checksum was recovered experimentally from PS7002 captures and verified against historical PS7200L example packets:

```text
checksum = sum(mfg[0:12]) & 0xFF
```

For:

```text
02 A1 09 FF 03 27 02 99 86 FF FF 21 15 AA
                                    ^^
```

the low byte of the sum of bytes 0 through 11 is `0x15`.

The historical Android path reviewed here does not appear to validate this checksum, but the research firmware does.

## 8. Complete reference frame

```text
02 A1 09 FF 03 27 02 99 86 FF FF 21 15 AA
```

Decoded:

```text
weight raw       807
weight           80.7 kg
health/Z         665
status           0x86
mode             0x21
scale class      body composition
resolution       0.1 kg
checksum         valid
```

## 9. Device addressing

One captured PS7002 advertised as:

```text
MAC  ff:ff:ff:ff:ff:d0
NAME YoHealth
```

The decoder deliberately does not depend on that MAC address. It identifies frames by protocol structure so the implementation can be tested with other units and related models.

## 10. Passive operation

No connection, pairing or outbound BLE command is required for the observed measurement flow. The implementation is receive-only, making the protocol suitable for future Passive BLE Monitor / Home Assistant integration.

## 11. Open protocol questions

- meaning of bytes 9–10;
- complete mapping of all status/error bits;
- unit-mode behavior beyond captured kg mode;
- whether all compatible devices use `02 A1 09 FF` framing;
- whether other Laica/YoHealth models use additional mode codes;
- whether some models transmit different final-state values while following the same bit logic.
