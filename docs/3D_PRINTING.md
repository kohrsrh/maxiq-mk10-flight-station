# 3D Printing the MaxIQ Model Rocket

The MaxIQ MK10 rocket body and xChip mounting hardware are fully 3D-printable. All STL files are provided in the official Domino4 repository.

## STL Files

**Download all STL files from:**
https://github.com/domino4com/Model-Rocket/tree/main/3D%20Files

This folder contains the printable parts for:
- Rocket body sections (nose cone, body tube, fin can)
- xChip stack mounting insert (holds CWA, IIA, IWB, PLA/PB02 securely during flight)
- Any payload bay or access hatch parts

## Recommended Print Settings

| Setting | Recommendation |
|---------|---------------|
| Material | **PETG** (preferred) or PLA |
| Layer height | 0.2mm |
| Infill | 20–30% |
| Perimeters/walls | 3 minimum |
| Supports | As needed (check slicer preview) |
| Bed adhesion | Brim recommended for tall/thin parts |

**Why PETG over PLA?**
PETG handles the heat inside a rocket airframe better than PLA, which can soften near warm motor casings. For low-power (A–C motors) flights, PLA is generally fine. For anything larger, use PETG or ABS.

## Assembly Notes

1. Print all parts and dry-fit before gluing anything
2. The xChip insert is sized to hold the snapped-together xChip stack (CWA + IIA + IWB + PLA/PB02) — insert the stack and confirm it sits firmly without rattling
3. The PPU programmer module is **not** included in the flight stack — remove it before inserting the electronics into the rocket
4. Use a small amount of hot glue or foam padding to prevent the xChip stack from shifting inside the insert
5. Route no wires through the airframe — the self-contained xChip stack runs entirely on battery power

## Matching the OpenRocket Model

The 3D files in the domino4com repo are matched to the `.ork` rocket configuration file in the `OpenRocket` folder. If you change any dimensions during printing (scaling up/down, thicker walls), update the corresponding values in OpenRocket so your simulations remain accurate.

See [OPENROCKET.md](OPENROCKET.md) for guidance on using the simulation file.
