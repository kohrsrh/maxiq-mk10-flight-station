# Data Download via Ymodem (domino4com Firmware)

This guide applies to the **official domino4com firmware** (https://github.com/domino4com/Model-Rocket), which stores flight data in the CWA's **internal flash memory** rather than on an SD card. Data retrieval uses the Ymodem protocol over a serial connection.

> If you are using the `Rocket_FlightStation.ino` firmware from **this repository**, your data is on the MicroSD card — just remove the card and read the CSV file on your computer. This guide is not needed for that case.

---

## What You Need

A serial terminal application that supports **Ymodem receive**:

| Platform | Recommended App |
|----------|----------------|
| macOS | [Serial](https://apps.apple.com/za/app/serial/id877615577?mt=12) (App Store, paid) |
| Windows | [TeraTerm](https://ttssh2.osdn.jp/) (free) or ExtraPuTTY (free) |

> Standard apps like the Arduino Serial Monitor do **not** support Ymodem. You must use one of the above.

---

## Step 1 — Prepare the Rocket (Before Flight)

Before flying with the domino4com firmware, the internal memory must be formatted:

1. Connect via PPU to computer
2. Press **⬅ + ➡** simultaneously on the CWA core (or press **A** in RemoteXY)
3. **RED LED flashes FAST** — formatting in progress (~35 seconds)
4. **BLUE LED flashes SLOWLY** — formatting complete, ready for flight

---

## Step 2 — Arm and Fly

1. Press **✔** on the CWA core (or **B** in RemoteXY) to arm
2. Keep rocket horizontal until the launch pad — no recording while horizontal
3. Make rocket vertical → **YELLOW/GREEN LED** = recording started
4. After flight and recovery: **BLUE LED flashes SLOWLY** = data saved

---

## Step 3 — Download the Data

1. Connect the CWA to your computer via the PPU
2. Open your Ymodem-capable serial terminal
3. Connect to the correct COM port at **115200-N-8-1**
4. Press **⬆ + ⬇** simultaneously on the CWA core (or **C** in RemoteXY)
5. The terminal displays: `Start the YMODEM download on your computer!`
6. **Immediately** start a Ymodem receive in your terminal:
   - **TeraTerm:** File > Transfer > YMODEM > Receive
   - **Serial (macOS):** Transfer > Receive File > YMODEM
7. The file `data.bin` transfers — this takes approximately **7 minutes**

> Don't wait after the prompt appears — start Ymodem immediately or the transfer will time out.

---

## Step 4 — Convert Binary to CSV

The downloaded `data.bin` file is in a custom binary format. Convert it using the Python script included in the domino4com repo:

```bash
python bin2csv.py -i data.bin -o data.csv
```

The `bin2csv.py` script is in the `python` folder of https://github.com/domino4com/Model-Rocket.

**Requirements:** Python 3 installed on your computer. Check with `python --version` in a terminal.

The output `data.csv` adds calculated columns for altitude and acceleration magnitude.

---

## Data Volumes

| Metric | Value |
|--------|-------|
| Formatted memory | 2,581 KB |
| Typical memory used | ~2.4 MB |
| Recorded duration | ~1 min 44 sec |
| Sample rate | ~1ms per record |
| Records | ~102,000 |
| Binary file size | ~2.3 MB |
| CSV file size | ~14.3 MB |
| Download time | ~7 minutes |

---

## LED Quick Reference

| LED | Meaning |
|-----|---------|
| RED flashing FAST | Formatting memory (wait ~35 sec) |
| BLUE flashing SLOW | Ready / flight data saved |
| YELLOW/GREEN solid | Recording in progress |
| No LED | Sleep mode (horizontal, waiting for vertical) |

---

## Button Reference

| Buttons | RemoteXY | Action |
|---------|----------|--------|
| ⬅ + ➡ | A | Format memory |
| ✔ | B | Arm for flight |
| ⬆ + ⬇ | C | Begin Ymodem download |
