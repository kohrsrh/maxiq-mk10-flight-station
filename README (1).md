# MaxIQ MK10 Rocket Flight Station

Firmware for the MaxIQ MK10 3D-printed model rocket. Records accelerometer, barometric pressure, altitude, temperature, and battery data to a CSV file on an SD card throughout the entire flight.

---

## Hardware Required

| Module | Chip | Purpose |
|--------|------|---------|
| CWA V2+ | ESP32-S3FN8 | Main controller core |
| IIA | LIS2DH12 or KXTJ3-1057 | Accelerometer (auto-detected) |
| IWB | SPL06-001 | Barometer / thermometer |
| PLA | AXP2101 | Power management / battery charging |
| EBS | — | SD card interface |

You will also need:
- A **MicroSD card** formatted as **FAT32**
- A **USB-C cable** for programming
- A computer with **Arduino IDE 2.x** installed

---

## Battery

The CWA core is powered by a **3.7V Li-ion battery** connected to the PLA power module via a **JST 2-pin connector**. The JST connector is a small, keyed 2-pin plug — it will only fit one way, so it cannot be plugged in backwards.

The PLA module (AXP2101 chip) handles battery charging automatically when the USB-C cable is connected, so you can charge the battery simply by plugging the rocket into any USB-C power source.

**Tips:**
- Make sure the battery is fully charged before flight
- The Serial Monitor reports battery voltage and percentage in every data line so you can confirm charge level before launch
- A capacity of **500mAh or greater** is recommended for reliable logging through a full flight session

---

## Step 1 — Install Arduino IDE

1. Download Arduino IDE 2 from [arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Install and open it

---

## Step 2 — Install ESP32 Board Support

1. In Arduino IDE, go to **File > Preferences**
2. In the *Additional boards manager URLs* box, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Click **OK**
4. Go to **Tools > Board > Boards Manager**
5. Search for `esp32` and install the package by **Espressif Systems** (version 3.x or later)

---

## Step 3 — Install Required Libraries

Go to **Sketch > Include Library > Manage Libraries** and install each of these:

| Library Name | Author | Search Term |
|---|---|---|
| SparkFun LIS2DH12 Arduino Library | SparkFun | `SparkFun LIS2DH12` |
| KXTJ3-1057 | ldab | `KXTJ3` |
| XPowersLib | lewisxhe | `XPowers` |
| Freenove WS2812 Lib for ESP32 | Freenove | `Freenove WS2812` |

> The SD and Wire libraries are built into the ESP32 board package — no extra install needed.

---

## Step 4 — Configure Board Settings

Plug in your CWA core via USB-C, then in Arduino IDE set the following under the **Tools** menu:

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 8MB (64Mb) |
| USB CDC on Boot | **Enabled** |
| Upload Speed | 921600 |
| Port | (whichever COM port appears when plugged in) |

> ⚠️ **USB CDC on Boot must be Enabled** or the Serial Monitor will show nothing.

> 💡 When preparing for an untethered flight, change *USB CDC on Boot* to **Disabled** — this prevents the board from waiting for a USB connection on startup. Re-enable it when you want to use the Serial Monitor again.

---

## Step 5 — Prepare the SD Card

1. Use a MicroSD card (up to 32GB works reliably)
2. Format it as **FAT32** on your computer
3. Insert it into the EBS module on the rocket

---

## Step 6 — Upload the Sketch

1. Open `Rocket_FlightStation.ino` in Arduino IDE
2. Verify all Tools settings from Step 4 are correct
3. Click the **Upload** button (right arrow icon)
4. Wait for "Done uploading" in the output panel

---

## Step 7 — Verify It's Working

1. After uploading, go to **Tools > Serial Monitor**
2. Set baud rate to **115200**
3. You should see startup messages followed by a stream of sensor readings like:
   ```
   === Rocket Flight Station v1.0 ===
   PLA Power... OK
   IIA Accel LIS2DH12... OK (LIS2DH12)
   IWB Baro... OK - 1013.2 hPa
   EBS SD Card... OK - /FLIGHT0.CSV
   === Setup Complete - On Pad ===

   [PAD] #1 | Accel: X0.01 Y0.02 Z1.00 (1.00g) | P:1013.2hPa T:22.3C Alt:0.0m | Batt:4150mV 95%
   ```

---

## LED Status Codes

| Neopixel Color | Meaning |
|---|---|
| 🔵 Blue | Initializing |
| 🟢 Green | All OK — ready on pad |
| 🟡 Yellow | Sensor warning (still logging) |
| 🔴 Red | SD card failed — no logging |
| ⚪ White | Launch detected |
| 🩵 Cyan | Apogee detected |
| 🟣 Purple | Descending |
| 🟠 Orange | Landed |

The **small red LED** on the CWA core blinks rapidly whenever data is being written to the SD card. This is normal and expected.

---

## Flight State Machine

The firmware automatically moves through these states:

```
PAD → LAUNCH → COAST → APOGEE → DESCENT → LANDED
```

| State | Trigger |
|---|---|
| PAD | Default on startup |
| LAUNCH | Acceleration ≥ 2.5g for 3 consecutive reads |
| COAST | Acceleration drops back below 2.5g |
| APOGEE | Pressure starts rising (altitude falling) |
| DESCENT | Immediately after apogee |
| LANDED | Acceleration stable near 1g for 10+ seconds |

---

## SD Card Data

Log files are named `FLIGHT0.CSV`, `FLIGHT1.CSV`, etc. — a new file is created each time the rocket powers on, so previous flights are never overwritten.

The CSV columns are:
```
Packet, Millis, State, AccelX_g, AccelY_g, AccelZ_g, AccelMag_g,
Pressure_hPa, Temp_C, RelAlt_m, BattVolt_mV, BattPct, Event
```

Logging runs from the moment of power-on through landing at 500ms intervals on the pad, capturing the full flight.

---

## Pre-Launch Checklist

- [ ] MicroSD card inserted and formatted FAT32
- [ ] All xChip modules firmly connected
- [ ] Battery plugged in
- [ ] Neopixel shows solid **green** (or yellow if a non-critical sensor is missing)
- [ ] Small red LED blinking (confirms SD writes are happening)
- [ ] USB-C disconnected and *USB CDC on Boot* set to **Disabled** for flight

---

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for solutions to known issues including ESP32-S3 crash loops.
