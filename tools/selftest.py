#!/usr/bin/env python3
from yohealth_calc import calculate

m = calculate(80.7, 665, 175.0, 55, True)

checks = {
    "BMI": (m.bmi, 26.351020408163266, 1e-6),
    "fat": (m.body_fat_pct, 23.286245353159853, 1e-6),
    "water": (m.water_pct, 56.0010408921933, 1e-6),
    "muscle": (m.muscle_pct, 38.73085501858736, 1e-6),
    "bone_candidate": (m.bone_candidate_kg, 2.856424, 1e-6),
}

for name, (got, expected, tol) in checks.items():
    if abs(got - expected) > tol:
        raise SystemExit(f"FAIL {name}: got {got}, expected {expected}")

if m.bmr_kcal != 1673:
    raise SystemExit(f"FAIL BMR: got {m.bmr_kcal}, expected 1673")
if m.body_age != 67:
    raise SystemExit(f"FAIL body age: got {m.body_age}, expected 67")

print("PASS: recovered reference vector matches expected values")
