# Offline Setup (alternative — USB SD card reader)

> **Most users:** use **[SETUP-GUIDE.md](SETUP-GUIDE.md)** instead — same SD card in Pi 3, no reader needed.

This guide is only if you have a **separate SD card** in a **USB reader** while the Pi 3 runs from its own SD card.

## The idea

| Machine | Role |
|---------|------|
| **Your PC** | Copy `oledtty` folder to USB pendrive |
| **Pi 3** (WiFi/ethernet) | Build oledtty, write it to the SD card, enable I2C in boot config |
| **Pi Zero 1.3** | Just boot — OLED becomes your mini monitor. **No install commands needed.** |

> **Important:** Do **not** run `install.sh` or `i2cdetect` on the Pi 3 expecting to see the OLED. The display is soldered to the Pi Zero, not the Pi 3. The Pi 3 only prepares the SD card files.

---

## What you need

- Raspberry Pi **Zero 1.3** + soldered 128×64 SSD1306 OLED (I2C, address `0x3C`)
- Raspberry Pi **3** (or 4/5) with **internet** (for `apt` build tools, one time)
- USB **pendrive** with the `oledtty` project folder
- **Micro SD card** with Raspberry Pi OS Lite flashed (on your PC via Raspberry Pi Imager)
- USB **SD card reader** (plug SD into Pi 3)

---

## Step 1 — Flash the SD card (on your PC)

1. Use [Raspberry Pi Imager](https://www.raspberrypi.com/software/) to flash **Raspberry Pi OS Lite** to the micro SD card
2. In Imager settings (gear icon), optionally set hostname / user / password
3. Copy the entire **`oledtty`** folder from this repo to a USB pendrive

---

## Step 2 — Prepare the SD card (on Pi 3)

1. Boot the **Pi 3** normally (its own SD card, with internet)
2. Plug in the **pendrive** and the **target SD card** (Pi Zero's card) via USB reader
3. The card should mount automatically. Check paths:

```bash
ls /media/$USER/
# Common layout from Raspberry Pi Imager:
#   /media/pi/rootfs   ← ext4 root partition
#   /media/pi/bootfs   ← FAT boot partition
```

4. Copy project from pendrive and run the prepare script:

```bash
cp -r /media/$USER/USB/oledtty ~/
cd ~/oledtty
chmod +x prepare-sdcard.sh zero-start.sh install.sh uninstall.sh

sudo ./prepare-sdcard.sh --root /media/$USER/rootfs --boot /media/$USER/bootfs
```

If mount names differ, adjust paths. Auto-detect also works if only the target SD is plugged in:

```bash
sudo ./prepare-sdcard.sh
```

### What the script does

1. Installs `build-essential` + `libi2c-dev` **on the Pi 3** (host only)
2. Compiles `oledtty` for **ARMv6** (Pi Zero compatible — critical!)
3. Writes I2C enable lines to the SD card's `config.txt`
4. Copies `/usr/local/bin/oledtty` onto the SD card root filesystem
5. Installs and **enables** the systemd service on the SD card
6. Does **not** run `i2cdetect` (OLED is not on Pi 3 — that is correct)

---

## Step 3 — Boot the Pi Zero (zero commands)

1. Safely eject the SD card from the Pi 3
2. Insert it into the **Pi Zero 1.3**
3. Connect the **OLED** (already soldered: VCC, GND, SCL, SDA)
4. Power on — **no HDMI, no WiFi, no commands**

Expected behavior:

1. Brief splash: `OLEDTTY … starting…`
2. Kernel boot messages scroll on the OLED
3. Login prompt appears on the OLED
4. Shell output mirrors in near real time

---

## Optional — one command on Pi Zero (fallback only)

If the OLED stays blank after boot (rare), connect a keyboard and run:

```bash
sudo ~/oledtty/zero-start.sh
```

Or if you copied the project to the Zero's home folder from pendrive:

```bash
cd ~/oledtty && sudo ./zero-start.sh
```

Normally this is **not needed** — `prepare-sdcard.sh` already enabled the service.

---

## Why build on Pi 3 but run on Pi Zero?

Pi Zero 1.3 uses an **ARMv6** CPU. Pi 3 uses **ARMv7**. A normal Pi 3 build **will not run** on Pi Zero.

This project's Makefile always uses:

```
-march=armv6 -mfpu=vfp -mfloat-abi=hard
```

`prepare-sdcard.sh` verifies the binary architecture before writing to the SD card.

---

## Wiring reminder (Pi Zero 1.3)

```
Pi Zero              SSD1306 OLED
───────              ────────────
3.3V (pin 1)  ────   VCC
GPIO 2 / SDA  ────   SDA
GPIO 3 / SCL  ────   SCL
GND  (pin 6)  ────   GND
```

---

## Troubleshooting on Pi Zero (no network)

| Problem | Fix |
|---------|-----|
| Blank OLED | Check wiring; keyboard login → `sudo systemctl status oledtty` |
| Service failed | `journalctl -u oledtty -n 30` |
| Wrong SD prep | Re-run `prepare-sdcard.sh` on Pi 3 |
| Restart display | `sudo systemctl restart oledtty` |

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more.

---

## Do NOT use on Pi 3 for final setup

| Command | On Pi 3? | On Pi Zero? |
|---------|----------|-------------|
| `prepare-sdcard.sh` | **Yes** — prepares SD card | No |
| `install.sh` | No (wrong machine) | Only if Zero has internet |
| `i2cdetect -y 1` | No (OLED not here) | Yes (optional debug) |

Back to [README](../README.md)
