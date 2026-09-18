# Validation data

`measurements-template.csv` is the canonical schema for experimental comparisons.
Copy it to a new file (for example `measurements-2026-09.csv`) and append one row
per completed weighing session.

Do not commit personally identifying names or unrelated health records.  The
project needs only the profile variables used by the algorithm (sex, age,
height) and the scale/app outputs required to validate formulas.

Recommended workflow:

1. Configure the sketch profile to exactly match the Laica app profile.
2. Start serial capture.
3. Perform one complete barefoot body-composition measurement.
4. Copy the ESP32 `FINAL MEASUREMENT` / `CSV` result.
5. Record the corresponding values shown by the app.
6. Add one row to the CSV and note any unusual condition.
