# Setup Guide — One SD Card, No SD Card Reader

This is the guide for your exact setup:

- **One micro SD card** (Raspberry Pi OS Lite flashed on your PC)
- **No SD card reader** — you move the same card between Pis
- **Pi 3** (or Pi 4) = setup machine with internet + USB pendrive
- **Pi Zero 1.3** = final machine with OLED soldered, **no WiFi**

---

## Before you start

### Flash the SD card (on your PC)

1. Use [Raspberry Pi Imager](https://www.raspberrypi.com/software/)
2. Choose **Raspberry Pi OS Lite (32-bit)** — required so the same card boots on Pi Zero 1.3
3. Flash to your micro SD card
4. Copy the entire **`oledtty`** folder to a USB pendrive

### Wiring (on Pi Zero — do this whenever convenient)

```
Pi Zero 1.3        SSD1306 OLED
───────────        ────────────
3.3V (pin 1)  ──   VCC
GPIO 2 / SDA  ──   SDA
GPIO 3 / SCL  ──   SCL
GND  (pin 6)  ──   GND
```

---

## Step 1 — Setup on Pi 3 (same SD card)

1. Insert the **SD card** into the **Pi 3**
2. Plug in **USB pendrive** (with `oledtty` folder)
3. Boot the Pi 3 and connect to internet (WiFi or ethernet)
4. Open a terminal and run:

```bash
# Find the pendrive name (often USB or the label you gave it)
ls /media/$USER/

# Copy the project off the pendrive
cp -r /media/$USER/USB/oledtty ~/
cd ~/oledtty

# One command — prepares everything on THIS sd card
chmod +x setup-for-pizero.sh
sudo ./setup-for-pizero.sh
```

Replace `USB` with your pendrive's folder name from `ls /media/$USER/`.

### What happens

| Step | What the script does |
|------|----------------------|
| Enables I2C | Writes to `/boot/firmware/config.txt` on the SD card |
| Installs tools | `build-essential`, `libi2c-dev` (needs internet) |
| Builds oledtty | **ARMv6** binary for Pi Zero 1.3 |
| Installs service | Binary + systemd unit, enabled on boot |
| Skips OLED test | No `i2cdetect` — OLED is on Pi Zero, not Pi 3 |

### Shut down

```bash
sudo shutdown -h now
```

Remove the SD card from the Pi 3.

---

## Step 2 — Pi Zero (main goal)

1. Insert the **same SD card** into **Pi Zero 1.3**
2. OLED should already be soldered (VCC, GND, SCL, SDA)
3. **Power on** — no HDMI, no WiFi, **no commands**

That's it. The OLED is your mini monitor:

- Boot messages appear during startup
- Login prompt shows on the OLED
- Shell commands and output mirror live

---

## If the OLED stays blank on Pi Zero

Connect a USB keyboard (USB OTG adapter on Zero) and run:

```bash
sudo systemctl restart oledtty
journalctl -u oledtty -f
```

Or:

```bash
sudo i2cdetect -y 1
```

You should see `3c` at address 0x3C. If not, check soldering.

---

## Command cheat sheet

| Where | Command |
|-------|---------|
| **Pi 3** (setup, once) | `sudo ./setup-for-pizero.sh` |
| **Pi Zero** (normally) | *nothing — just power on* |
| **Pi Zero** (fallback) | `sudo systemctl restart oledtty` |

---

## Do NOT do this

| Wrong | Why |
|-------|-----|
| Run `i2cdetect` on Pi 3 expecting to see OLED | Display is on Pi Zero |
| Use `install.sh` on Pi 3 | It tries to start the service immediately |
| Flash 64-bit OS | Pi Zero 1.3 needs **32-bit** image |
| Copy only `src/` folder | Copy the whole `oledtty` folder |

---

## Files on the pendrive

Copy this entire folder:

```
oledtty/
├── setup-for-pizero.sh    ← run this on Pi 3
├── Makefile
├── oledtty.service
├── src/
├── include/
└── docs/
```

Back to [README](../README.md)
