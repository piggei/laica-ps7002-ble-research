#!/usr/bin/env python3
"""Reference calculator for the recovered YoHealth getHealth() algorithm.

The formulas were reconstructed from historical native-code artifacts. Their
user-facing field mapping was recovered from the historical Android application
that consumed getHealth(). The implementation is independent from the ESP32
sketch so results can be checked offline and regression-tested.
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import asdict, dataclass


@dataclass
class Metrics:
    weight_kg: float
    impedance: int
    bmi: float
    lean_mass_kg: float
    body_fat_pct: float
    water_pct: float
    muscle_pct: float
    bone_mass_kg: float
    visceral_fat_pct: float
    body_age: int
    bmr_kcal: int


def calculate(
    weight_kg: float,
    impedance: int,
    height_cm: float,
    age: int,
    male: bool,
) -> Metrics:
    """Calculate the complete historical YoHealth body-composition result.

    Args:
        weight_kg: Scale weight in kilograms.
        impedance: Raw 16-bit YoHealth health/impedance value.
        height_cm: Profile height in centimetres.
        age: Profile age in whole years.
        male: True for male (native sex=0), False for female (native sex=1).
    """

    sex = 0.0 if male else 1.0
    height_m = height_cm / 100.0
    bmi = weight_kg / (height_m * height_m)

    lean = (
        0.00067 * height_cm * height_cm
        + 2.0
        + 0.53 * weight_kg
        - 0.00095 * impedance
        - 3.0 * sex
        - 0.05 * age
    )

    fat = (weight_kg - lean) / weight_kg
    if fat < 0.10:
        fat = fat + 0.7 * (0.10 - fat)

    water = 0.73 * lean / weight_kg

    if male:
        muscle_pct = ((7.78 * height_cm + 334.0 - 9.8 * age) / weight_kg) + 24.4
        # Native code calls C round() before converting to integer. All BMR
        # values are positive, so floor(x + 0.5) reproduces that behavior.
        bmr = math.floor(
            13.7 * weight_kg + 5.0 * height_cm - 6.8 * age + 66.0 + 0.5
        )
    else:
        muscle_pct = ((7.74 * height_cm - 318.0 - 9.8 * age) / weight_kg) + 24.4
        bmr = math.floor(
            9.6 * weight_kg + 1.8 * height_cm - 4.7 * age + 655.0 + 0.5
        )

    # getHealth() emits 30 * bone_mass as CSV field 4. Historical Android code
    # divides that field by 30 and names the resulting value boneMass.
    bone_mass = (
        0.0077200001 * weight_kg
        + 0.0045 * height_cm
        + 1.95
        - 0.00636 * age
        - 0.000232 * impedance
    )
    if not male:
        bone_mass *= 0.75

    # getHealth() field 5 is a fraction. Historical Android code multiplies by
    # 100 and passes it to stableData() as visceralFatPercentage.
    visceral_fat_pct = fat * (0.45 if male else 0.20) * 100.0

    if age < 20:
        body_age = age
    elif bmi > 28.0:
        body_age = age + 15
    elif bmi > 26.0:
        body_age = age + 12
    elif bmi > 25.0:
        body_age = age + 7
    elif bmi > 23.0:
        body_age = age + 4
    elif age < 30:
        body_age = 18
    elif age <= 44:
        body_age = age - 12
    else:
        body_age = age - 16

    return Metrics(
        weight_kg=weight_kg,
        impedance=impedance,
        bmi=bmi,
        lean_mass_kg=lean,
        body_fat_pct=fat * 100.0,
        water_pct=water * 100.0,
        muscle_pct=muscle_pct,
        bone_mass_kg=bone_mass,
        visceral_fat_pct=visceral_fat_pct,
        body_age=body_age,
        bmr_kcal=int(bmr),
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Calculate recovered YoHealth body-composition metrics"
    )
    parser.add_argument("--weight", type=float, required=True, help="weight in kg")
    parser.add_argument(
        "--impedance",
        type=int,
        required=True,
        help="raw YoHealth health/impedance value",
    )
    parser.add_argument("--height", type=float, required=True, help="height in cm")
    parser.add_argument("--age", type=int, required=True, help="age in whole years")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--male", action="store_true")
    group.add_argument("--female", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    metrics = calculate(
        args.weight, args.impedance, args.height, args.age, args.male
    )

    if args.json:
        print(json.dumps(asdict(metrics), indent=2))
        return

    for key, value in asdict(metrics).items():
        if isinstance(value, float):
            print(f"{key:22s}: {value:.6f}")
        else:
            print(f"{key:22s}: {value}")


if __name__ == "__main__":
    main()
