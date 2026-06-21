# Deploy oledtty v2.0.3 (Pi Zero, no WiFi)

Your workflow: **PC → pendrive → Pi 3 (internet) → SD card → Pi Zero**.

The Pi Zero never needs network. Everything is installed onto the SD card on the Pi 3.

---

## What you need on the pendrive

Copy the **entire** `oledtty` folder. Required contents:

| Path | Purpose |
|------|---------|
| `setup-for-pizero.sh` | **Main script** — run on Pi 3 |
| `fix-line-endings.sh` | Fixes Windows CRLF (auto-run by setup) |
| `install.sh` | Direct install if OLED is on same Pi |
| `zero-start.sh` | Restart service on Pi Zero (optional) |
| `uninstall.sh` | Remove service |
| `Makefile` | Build |
| `oledtty.service` | Boot service |
| `include/` + `src/` | C source |

Not required on the Pi Zero: `tests/`, `docs/`, `prepare-sdcard.sh`.

The old `scripts/` and `config/` folders were **optional HDMI color tweaks** — not needed for the OLED. Do not restore them.

---

## Step 1 — Pi 3 (with internet + pendrive + SD card)

```bash
cp -r /media/$USER/USB/oledtty ~/
cd ~/oledtty
chmod +x setup-for-pizero.sh fix-line-endings.sh
sudo ./setup-for-pizero.sh
sudo shutdown -h now
```

This enables I2C, builds for ARMv6, installs `oledtty` + systemd service onto the SD card, disables competing `oled.service`.

---

## Step 2 — Pi Zero

1. Move SD card to Pi Zero 1.3 (OLED soldered)
2. Power on — **no commands needed**

---

## Step 3 — Verify (keyboard on Pi Zero, or SSH)

```bash
sudo systemctl disable --now oled.service 2>/dev/null || true
systemctl status oledtty
which oledtty
journalctl -u oledtty -n 20 --no-pager
```

**Expect:** `oledtty 2.0.3` and **8 lines of white text** on the OLED.

Run **oledtty** (C binary at `/usr/local/bin/oledtty`), **not** `oled_terminal.py`.

---

## If OLED is blank

```bash
sudo systemctl restart oledtty
# or
sudo ./zero-start.sh
```

If scripts fail with `env: 'bash\r'`:

```bash
cd ~/oledtty
chmod +x fix-line-endings.sh
./fix-line-endings.sh
sudo ./install.sh
```

---

## Direct install (Pi with OLED on same board + internet)

```bash
cd ~/oledtty
chmod +x install.sh fix-line-endings.sh
sudo ./install.sh
```

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more.
