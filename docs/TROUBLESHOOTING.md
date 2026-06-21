# Troubleshooting

Common problems and how to fix them.

---

## Script error: `env: 'bash\r': No such file`

Scripts were copied from Windows with CRLF line endings. Fix on the Pi:

```bash
cd ~/oledtty
chmod +x fix-line-endings.sh
./fix-line-endings.sh
```

Or: `find ~/oledtty -name '*.sh' -exec sed -i 's/\r$//' {} +`

`setup-for-pizero.sh` and `install.sh` run this automatically when you use them.

---

## Blank OLED after install

1. **Disable competing OLED programs:**
   ```bash
   sudo systemctl disable --now oled.service 2>/dev/null || true
   sudo systemctl enable --now oledtty
   ```
2. **Wiring** — VCC (3.3V), GND, SCL (GPIO 3), SDA (GPIO 2)
3. **I2C enabled** — `sudo raspi-config` → Interface Options → I2C
4. **Device detected:** `sudo i2cdetect -y 1` — expect `3c`
5. **Service running:** `systemctl status oledtty`
6. **Console on tty1:** `sudo fgconsole` — must print `1`

---

## White blocks but no letters (cursor only)

Rebuild and reinstall (v2.0.3 fixes Kali large-console blank screen):

```bash
cd ~/oledtty
make clean && make
sudo ./install.sh
sudo systemctl restart oledtty
journalctl -u oledtty -n 20 --no-pager
```

Expect `oledtty 2.0.3`. If you see `console grid NxM clamped to 30x80 window` that is normal on Kali.

```bash
which oledtty
systemctl status oledtty
```

---

## `E: Unable to locate package con2fbmap`

Fixed in v2.0.2 — `con2fbmap` is no longer required. Copy the updated `oledtty` folder and run `sudo ./install.sh` again.

---

## Permission denied on `/dev/vcsa1`

The service must run as **root**. Reinstall with `sudo ./install.sh`.

---

## Shift+Ctrl+Up/Down doesn't pan

Check `journalctl -u oledtty -n 20` for keyboard detection. SSH fallback: `oledtty-ctl up` / `down` / `live`.

---

## Garbled or flickering display

Lower I2C speed in `/boot/firmware/config.txt`:

```ini
dtparam=i2c_arm_baudrate=400000
```

Or try `100000`. Check wiring and use 3.3V power.

---

## Develop / test without the OLED

```bash
make
sudo ./build/oledtty --dry-run --once
```

---

Back to [README](../README.md)
