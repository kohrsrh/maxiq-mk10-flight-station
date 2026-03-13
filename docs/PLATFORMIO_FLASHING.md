# Flashing Firmware with PlatformIO

PlatformIO is an alternative to Arduino IDE for uploading firmware to the CWA core. It handles library dependencies automatically and is generally more reliable for complex projects. The official Domino4 rocket firmware is distributed as a PlatformIO project.

## When to Use PlatformIO vs Arduino IDE

| | Arduino IDE | PlatformIO |
|--|-------------|------------|
| Best for | This repo's `Rocket_FlightStation.ino` | Official domino4com firmware |
| Library install | Manual (Library Manager) | Automatic from `platformio.ini` |
| Board config | Manual (Tools menu) | Automatic |
| Skill level | Beginner-friendly | Intermediate |

For the `Rocket_FlightStation.ino` sketch in this repository, **Arduino IDE** is the recommended method (see the main README). PlatformIO is documented here for users who want to also use or study the official Domino4 firmware source.

## Setup

### 1. Install Visual Studio Code
Download and install from: https://code.visualstudio.com/

### 2. Install the PlatformIO Extension
1. Open VS Code
2. Click the Extensions icon in the left sidebar (or press `Ctrl+Shift+X`)
3. Search for `PlatformIO IDE`
4. Click **Install**
5. Restart VS Code when prompted

### 3. Download the Official Firmware
Clone or download the Domino4 rocket firmware:
https://github.com/domino4com/Model-Rocket

To download as a ZIP: click the green **Code** button → **Download ZIP** → extract to a folder on your computer.

### 4. Open the Project
1. In VS Code, go to **File > Open Folder**
2. Navigate to the extracted `Model-Rocket` folder
3. PlatformIO will automatically detect the `platformio.ini` file and configure the project

### 5. Erase Flash Before Uploading (Important)
The official firmware stores flight data in internal flash memory. Before uploading new firmware, erase the flash first:

```bash
python erase_before_upload.py
```

This script is included in the root of the domino4com repo. Run it from a terminal/command prompt in the project folder. You will need Python 3 installed.

### 6. Connect and Upload
1. Connect the CWA via PPU (USB-A cable) for CWA V1, or USB-C directly for CWA V2+
2. In VS Code, look for the PlatformIO toolbar at the **bottom** of the window
3. Click the **→ Upload** button (right-arrow icon)
4. PlatformIO will download any missing libraries, compile, and upload automatically

## Board Settings (Already in platformio.ini)

The `platformio.ini` file in the domino4com repo already contains the correct settings. You do not need to configure anything manually.

## Key Difference: domino4com Firmware vs This Repo's Firmware

The two firmware options behave differently:

| Feature | This repo (`Rocket_FlightStation.ino`) | domino4com firmware |
|---------|----------------------------------------|---------------------|
| Data storage | MicroSD card (EBS module) | Internal flash memory |
| Download method | Remove SD card and read on computer | Ymodem serial transfer |
| Core version | CWA V2+ (ESP32-S3) | CWA V1 (ESP32-WROOM-32) |
| Flight state machine | Automatic (accel + baro) | Button-triggered |
| Required extra hardware | EBS SD module | None (uses internal memory) |

See [DATA_DOWNLOAD_YMODEM.md](DATA_DOWNLOAD_YMODEM.md) for how to retrieve data using the domino4com firmware's Ymodem download method.
