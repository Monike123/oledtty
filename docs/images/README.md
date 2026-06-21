# Project images & diagrams

All visual assets for the README and documentation.

## Photos (real hardware)

| File | Description | Used in |
|------|-------------|---------|
| `hero-terminal-mirror.png` | OLED showing live shell (`uname`, `df`, prompt) | README hero |
| `hardware-pi-zero-setup.jpg` | Pi Zero held with OLED + HDMI dev setup | README gallery |
| `font-5x7-charset.png` | Embedded font glyph reference | DISPLAY.md |

## Diagrams (generated reference)

| File | Description | Used in |
|------|-------------|---------|
| `system-architecture.png` | Full system block diagram | README, ARCHITECTURE.md |
| `data-pipeline.png` | Per-frame software pipeline | README, ARCHITECTURE.md |
| `display-layout-grid.png` | 128×64 grid with 8×21 cells | README, DISPLAY.md |
| `viewport-scroll.png` | Console → capture window → OLED viewport | README, DISPLAY.md |
| `vcsa-memory-layout.png` | `/dev/vcsa1` byte format | README, ARCHITECTURE.md |
| `performance-chart.png` | Frame timing budget bar chart | README, ARCHITECTURE.md |
| `deploy-workflow.png` | PC → Pi 3 → Pi Zero deploy flow | README, ARCHITECTURE.md |
| `wiring-diagram.png` | I2C wire connections overview | README, HARDWARE.md |
| `gpio-pinout.png` | Pi Zero GPIO pin numbers | README, HARDWARE.md |

## Optional photos you can add

| Suggested filename | What to shoot |
|--------------------|---------------|
| `boot-splash.jpg` | Kernel boot lines on OLED |
| `login-prompt.jpg` | Login screen mirrored |
| `i2cdetect.jpg` | Terminal showing `3c` at address 0x3c |
| `soldered-closeup.jpg` | Close-up of GPIO solder joints |

**Tips:** good lighting, OLED text in focus, landscape ~1200px wide.

## GitHub social preview

**Settings → General → Social preview** → upload `hero-terminal-mirror.png`.

## Suggested repo topics

`raspberry-pi` `pi-zero` `oled` `ssd1306` `i2c` `headless` `linux-console` `embedded` `c` `tty`
