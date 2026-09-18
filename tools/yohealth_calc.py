#!/usr/bin/env python3
"""Reference calculator for the recovered YoHealth body-composition formulas.

This is intentionally independent from the ESP32 sketch so measurements can be
rechecked offline and formula changes can be regression-tested.
"""

from __future__ import annotations

import argparse
import math
from dataclasses import dataclass, asdict
import json


@dataclass
class Metrics:
    weight_kg: float
    impedance: int
    bmi: float
    lean_mass_kg: float
    body_fat_pct: float
    water_pct: float
    muscle_pct: float
    bone_candidate_kg: float
    bmr_kcal: int
    body_age: int
    native_metric_x: float
    native_metric_y: float


def calculate(weight_kg: float, impedance: int, height_cm: float, age: int, male: bool) -> Metrics:
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
        bmr = math.floor(13.7 * weight_kg + 5.0 * height_cm - 6.8 * age + 66.0 + 0.5)
    else:
        muscle_pct = ((7.74 * height_cm - 318.0 - 9.8 * age) / weight_kg) + 24.4
        bmr = math.floor(9.6 * weight_kg + 1.8 * height_cm - 4.7 * age + 655.0 + 0.5)

    bone = (
        0.0077200001 * weight_kg
        + 0.0045 * height_cm
        + 1.95
        - 0.00636 * age
        - 0.000232 * impedance
    )
    if not male:
        bone *= 0.75

    native_x = 30.0 * bone
    native_y = fat * (0.45 if male else 0.20)

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
        bone_candidate_kg=bone,
        bmr_kcal=int(bmr),
        body_age=body_age,
        native_metric_x=native_x,
        native_metric_y=native_y,
    )


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--weight", type=float, required=True, help="weight in kg")
    p.add_argument("--impedance", type=int, required=True, help="raw YoHealth health/impedance value")
    p.add_argument("--height", type=float, required=True, help="height in cm")
    p.add_argument("--age", type=int, required=True)
    group = p.add_mutually_exclusive_group(required=True)
    group.add_argument("--male", action="store_true")
    group.add_argument("--female", action="store_true")
    p.add_argument("--json", action="store_true")
    args = p.parse_args()

    m = calculate(args.weight, args.impedance, args.height, args.age, args.male)
    if args.json:
        print(json.dumps(asdict(m), indent=2))
        return

    for k, v in asdict(m).items():
        if isinstance(v, float):
            print(f"{k:22s}: {v:.6f}")
        else:
            print(f"{k:22s}: {v}")


if __name__ == "__main__":
    main()
