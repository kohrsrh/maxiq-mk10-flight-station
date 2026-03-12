# Troubleshooting Guide

This document covers known issues encountered when building and flashing the MaxIQ MK10 Flight Station firmware on the CWA V2+ (ESP32-S3) core.

---

## Issue 1 — Continuous Crash / Reboot Loop (Guru Meditation Error)

**Symptom:**
- Neopixel turns blue briefly, then starts blinking red and yellow
- Serial monitor shows repeating `Guru Meditation Error: Core 1 panic'ed (LoadProhibited)` followed by `Rebooting...`
- The board never reaches the green "ready" state

**Example output:**
```
Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
EXCVADDR: 0x03ced76c
Rebooting...
```

**Root Cause:**

The ESP32-S3 crashes when C++ objects that interact with hardware (I2C, SPI, GPIO) are constructed at **global scope** — before the Arduino `setup()` function runs and before `Wire.begin()` initializes the I2C bus. When these constructors try to access hardware, they dereference invalid memory addresses (identifiable by the `0x03xxxxxx` pattern in `EXCVADDR`), causing a fatal exception.

Three objects in earlier versions of this firmware had this problem:
1. `Freenove_ESP32_WS2812 strip(...)` — Neopixel, touches GPIO at construction
2. `SPARKFUN_LIS2DH12 accelLIS` — Accelerometer, touches I2C at construction
3. `KXTJ3 accelKX(KXTJ3_ADDR)` — Accelerometer, touches I2C at construction

A fourth issue was a scoping bug with the SD card's SPI bus object being destroyed after `setup()` returned.

**Fix Applied:**

All hardware objects are now declared as `nullptr` pointers at global scope and constructed with `new` inside `setup()` after `Wire.begin()` has run. The SPI bus object for the SD card is also heap-allocated so it persists through `loop()`.

The current version of `Rocket_FlightStation.ino` in this repository already contains all of these fixes. If you are starting fresh, simply use the provided file and you will not encounter this issue.

---

## Issue 2 — Serial Monitor Shows Nothing

**Symptom:**
- Board appears to be running (LEDs active) but Serial Monitor is completely blank

**Cause:**

The ESP32-S3 requires **USB CDC on Boot** to be enabled in order to use the USB-C port as a serial monitor. If this setting is Disabled, the USB serial output is suppressed.

**Fix:**

In Arduino IDE, go to **Tools > USB CDC on Boot > Enabled**, then re-upload the sketch.

> Note: For untethered flight (no USB connected), set this back to **Disabled** so the board doesn't stall waiting for a USB host on startup. Re-enable it any time you want to use the Serial Monitor.

---

## Issue 3 — Neopixel Shows Red (SD Card Failed)

**Symptom:**
- Neopixel glows solid red after initialization
- Serial monitor shows: `INIT FAILED - CHECK SD CARD` or `FILE OPEN FAILED`

**Possible Causes and Fixes:**

| Cause | Fix |
|---|---|
| No SD card inserted | Insert a MicroSD card into the EBS module |
| SD card not formatted correctly | Reformat as FAT32 on your computer |
| SD card larger than 32GB | Use a 32GB or smaller card; larger cards may use exFAT which is not supported |
| EBS module not firmly connected | Press all xChip modules together firmly until they click |
| Corrupted filesystem | Reformat the card |

---

## Issue 4 — Neopixel Shows Yellow (Sensor Warning)

**Symptom:**
- Neopixel glows yellow after initialization
- Serial monitor shows one or more `NOT FOUND` messages for sensors

**What it means:**

Yellow means at least one non-critical sensor failed to initialize, but the firmware is still running and logging. The SD card is working.

Check the Serial Monitor output to see which sensor reported `NOT FOUND`:

- `PLA Power... NOT FOUND` — AXP2101 power management chip not detected. Check PLA module connection.
- `IIA Accel... NOT FOUND` — Neither accelerometer was found. Check IIA module connection. Launch detection will not work.
- `IWB Baro... NOT FOUND` — Barometer not found. Check IWB module connection. Altitude will read 0.

In all cases, firmly re-seat the affected xChip module and power cycle the board.

---

## Issue 5 — Compilation Error: No Matching Function for XPowersAXP2101

**Symptom:**
```
error: no matching function for call to 'XPowersAXP2101::XPowersAXP2101(...)'
```

**Cause:**

Older or modified versions of this firmware used incorrect constructor signatures for the XPowersLib. The correct constructor for this library on ESP32-S3 is:

```cpp
pmu = new XPowersAXP2101(Wire, I2C_SDA, I2C_SCL, AXP2101_ADDR);
if (pmu->init()) { ... }
```

**Fix:**

Use the `Rocket_FlightStation.ino` file provided in this repository — it already uses the correct constructor.

---

## Issue 6 — Board Not Appearing in Arduino IDE Port List

**Symptom:**
- No COM port appears under Tools > Port when the board is plugged in

**Fixes to try:**
1. Try a different USB-C cable — many cables are charge-only and do not carry data
2. Try a different USB port on your computer
3. On Windows, check Device Manager for an unknown device and install the CP210x or CH340 driver if prompted
4. Hold the **BOOT** button on the CWA core while plugging in, then release — this forces the board into download mode

---

## General Tips

- Always open the Serial Monitor at **115200 baud**
- Power cycle the board (unplug and replug) after changing the *USB CDC on Boot* setting
- The small red LED blinking rapidly is **normal** — it indicates SD card writes are happening
- Pull the SD card and check for `FLIGHT0.CSV` on your computer to confirm logging is working before flight
