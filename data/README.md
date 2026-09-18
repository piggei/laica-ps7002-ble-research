# Validation data

`measurements-template.csv` is the canonical schema for experimental comparisons.

Copy it to a new file and append one row per completed weighing session.

Do not commit names, exact dates of birth or unrelated health records. The project needs the scale/app variables required to reproduce calculations and protocol behavior.

Recommended workflow:

1. Configure the profile to match the app.
2. Start serial capture.
3. Perform one complete body-composition measurement.
4. Copy the ESP32 `FINAL MEASUREMENT` / `CSV` output.
5. Record the values actually shown by the app.
6. Mark hidden fields as `not displayed` rather than guessing them.
7. Add one row to the dataset.

The current PS7002 app used in this project may hide bone mass, visceral fat and body age, while the recovered historical YoHealth algorithm still calculates them.
