# OpenRocket — Simulation & Post-Flight Analysis

OpenRocket is a free, open-source model rocket flight simulator. It lets you simulate your rocket before launch to predict altitude, velocity, and acceleration, and compare those predictions against your actual recorded flight data afterward.

## Download OpenRocket

**https://openrocket.info/**

OpenRocket requires Java. If the installer asks for Java, download the latest JDK from https://adoptium.net/ and install it first.

## Rocket Configuration File

The official Domino4 repo includes an `.ork` file that matches the 3D-printed rocket design exactly:

**https://github.com/domino4com/Model-Rocket/tree/main/OpenRocket**

Download the `.ork` file from that folder. This file contains the rocket's dimensions, mass, center of gravity, fin geometry, and nose cone profile — everything OpenRocket needs to simulate a flight.

## Running a Pre-Flight Simulation

1. Open OpenRocket
2. Go to **File > Open** and load the `.ork` file
3. Click the **Motor & Configuration** tab
4. Click **Add motor** and select your motor from the database (e.g., Estes B6-4, C6-5, etc.)
5. Click **Simulate** in the toolbar
6. Review the predicted **altitude**, **velocity**, and **acceleration** curves

This simulation tells you:
- Whether the rocket is aerodynamically stable (stability margin should be 1–2 calibers)
- Predicted apogee altitude
- Whether the ejection delay on your motor is correct for deploying the recovery system at apogee

## Post-Flight Analysis

After recovering the rocket and downloading your data, you can overlay the real flight data against the simulation.

### Using SD Card Data (This Repo's Firmware)

1. Remove the MicroSD card from the EBS module and read the `FLIGHT0.CSV` (or similar) file on your computer
2. Open the CSV in Excel, Google Sheets, or Python/matplotlib
3. Plot the `RelAlt_m` column against time for the altitude trace
4. Plot `AccelMag_g` against time for the total acceleration
5. Compare these curves to the OpenRocket simulation output

### Using Internal Flash Data (domino4com Firmware)

If you used the domino4com firmware instead, you'll have a `data.csv` file after Ymodem download and conversion. See [DATA_DOWNLOAD_YMODEM.md](DATA_DOWNLOAD_YMODEM.md) for the download process.

The converted CSV from the domino4com firmware includes:
- Raw accelerometer axes (X, Y, Z)
- Calculated acceleration magnitude
- Barometric altitude

Plot the altitude and magnitude columns the same way as above.

## Typical Flight Data Numbers

These are representative values from a real flight with the MaxIQ xChip stack:

| Metric | Value |
|--------|-------|
| Recording duration | ~1 min 44 sec |
| Sample rate | ~1ms per record |
| Total records | ~102,000 |
| Typical apogee (Estes C motor) | 50–150m depending on rocket weight |

## Tips for Good Simulation-to-Reality Comparison

- **Weigh the fully assembled rocket** (with motor casing, no propellant) and enter that mass in OpenRocket under the *Override* tab — 3D-printed parts often weigh more than the defaults
- **Use the correct motor** — the motor selected in OpenRocket must match what you actually flew
- **Barometric altitude is relative** — the firmware records altitude relative to the pad elevation (zeroed at power-on), which matches OpenRocket's output
- **Expect some noise** — real barometric data is noisier than a smooth simulation curve, especially near the ground
