# Installation Guide

> **Pi Zero without WiFi?** Use **[OFFLINE-SETUP.md](OFFLINE-SETUP.md)** instead — prepare the SD card on a Pi 3 with `prepare-sdcard.sh`. Do not use this guide for that workflow.

Step-by-step setup for a Pi Zero **with internet**, or any Pi where you build and run on the same board.

---

## What you need

| Item | Notes |
|------|-------|
| Raspberry Pi Zero 1.3 | Also works on Pi Zero W, Pi 3/4/5 |
| 0.91–0.96" SSD1306 OLED | 4-pin I2C, address usually **0x3C** |
| Soldered wiring | VCC, GND, SCL (GPIO 3), SDA (GPIO 2) |
| Raspberry Pi OS | Console on **tty1** (default on Lite) |
| USB pendrive | Optional — to copy the project without network |

---

## Step 1 — Get the project onto the Pi

### From a USB pendrive

**On your PC:** copy the entire `oledtty` folder to the USB drive. The folder must contain these at its top level:

```
oledtty/
├── Makefile
├── install.sh
├── uninstall.sh
├── oledtty.service
├── src/
└── include/
```

**On the Pi:**

```bash
# List mounted drives to find the path
ls /media/pi/

# Copy the project
cp -r /media/pi/YOUR_USB_NAME/oledtty ~/
cd ~/oledtty
```

### From Git

```bash
git clone https://github.com/Monike123/oledtty.git
cd oledtty
```

---

## Step 2 — Enable I2C (one-time)

```bash
sudo raspi-config
# Interface Options → I2C → Enable → Finish
```

Add bus speed to config (recommended):

```bash
sudo nano /boot/firmware/config.txt    # Bookworm
# or /boot/config.txt on older OS
```

```ini
dtparam=i2c_arm=on
dtparam=i2c_arm_baudrate=400000
```

Reboot and verify:

```bash
sudo reboot
sudo i2cdetect -y 1    # expect 3c at address 0x3C
```

Full wiring guide: [HARDWARE.md](HARDWARE.md)

---

## Step 3 — Build and install

```bash
cd ~/oledtty
chmod +x install.sh uninstall.sh
sudo ./install.sh
```

The installer:

1. Installs `build-essential`, `i2c-tools`, `libi2c-dev`
2. Compiles `build/oledtty`
3. Installs binary to `/usr/local/bin/oledtty`
4. Installs and enables the systemd service

---

## Step 4 — Verify

```bash
systemctl status oledtty
journalctl -u oledtty -f
```

**Expected:**

1. Splash screen: `OLEDTTY … starting…`
2. Live tty1 mirror — boot text, login prompt, shell output

**Test without hardware (over SSH):**

```bash
make
sudo ./build/oledtty --dry-run --once
```

---

## Step 5 — Use without HDMI

1. Power on the Pi — OLED connected, no HDMI needed
2. Watch boot messages on the OLED
3. Log in at tty1 (keyboard) or via SSH
4. Shell output mirrors to the display automatically

---

## Manual run (no service)

```bash
cd ~/oledtty
make
sudo ./build/oledtty
```

Press `Ctrl+C` to stop.

---

## Update to a newer version

```bash
cp -r /media/pi/USB/oledtty ~/oledtty    # or git pull
cd ~/oledtty
sudo ./install.sh
```

---

## Uninstall

```bash
cd ~/oledtty
sudo ./uninstall.sh
```

---

## Next steps

- [HARDWARE.md](HARDWARE.md) — wiring and I2C tuning
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) — if something goes wrong
- [PERFORMANCE.md](PERFORMANCE.md) — make updates faster

Back to [README](../README.md)
