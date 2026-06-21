# Changelog

All notable changes to this project are documented here.

## [2.0.3] - 2026-06-21

### Fixed

- **Blank OLED on Kali / large consoles** — vcsa grids like 86×131 were read with wrong stride; cursor row was outside the buffer so the viewport showed only spaces. Now reads row-by-row and keeps a 30×80 window around the cursor.

## [2.0.2] - 2026-06-21

### Fixed

- **Install on Kali / Pi OS** — removed `con2fbmap` dependency (not in all repos)
- **Windows CRLF scripts** — all `.sh` files use Unix LF and `#!/bin/bash`
- **Blank OLED / cursor-only** — restored v2.0.1 render path: white text, invert cursor, **full framebuffer flush every frame**
- **Stale frame** — keeps last terminal content when `/dev/vcsa1` read fails briefly

### Removed

- `con2fbmap` from service and install scripts
- Monochrome HDMI profile scripts (`scripts/`, `config/` — not needed for OLED)
- v2.1 partial-flush, underline cursor, and viewport anchor complexity
- Obsolete dev docs (`PROJECT_PLAN`, `TERMINAL_PLAN`, `ARCHITECTURE`, etc.)

### Added

- `fix-line-endings.sh` — strips Windows CRLF before build (auto-run in install scripts)
- `docs/DEPLOY.md` — single Pi 3 → Pi Zero guide (no WiFi on Zero)

## [2.1.1] - 2026-06-21

### Fixed

- **Blank OLED after login** — live view now anchors to the last non-empty terminal row (handles `clear` + shell prompt)
- **Pan controls stuck on black screen** — Shift+Ctrl+Down and Ctrl+Left return to live mode; horizontal pan is clamped so the prompt cannot scroll off-screen
- **Manual pan recovery** — typing or cursor movement returns to live follow; vcsa read recovery forces full redraw

## [2.1.0] - 2026-06-21

### Fixed

- **Blank OLED / cursor-only display** — verified 5×5 column-major font; white text only; cursor underline instead of invert block
- **Reliable I2C flush** — full page flush on content change and for the first 3 seconds after boot
- **Console readiness** — retry `/dev/vcsa1` for up to 5 seconds at startup
- **Startup self-test** — shows `OLEDTTY v2.1.0` for 1 second to verify font and I2C before mirroring

### Added

- **7-line viewport** — 5×5 font in 6×6 cells (21 cols × 7 rows on 128×64)
- **33 ms poll interval** (~30 fps cap) for responsive typing on Pi Zero
- Offline smoke test: `make test` with synthetic `tests/sample.vcsa`
- Setup script disables competing `oled.service` and quiets kernel spam on tty1

### Changed

- systemd service uses `--no-splash` (self-test still runs)
- Replaced `font5x8` with `font5x5`

## [2.0.1] - 2026-06-21

### Fixed

- Blank OLED / no text — removed broken partial-row render and color/bold styling
- Simple white monochrome text only; full-screen redraw each frame
- Keep last terminal frame when `/dev/vcsa1` read fails briefly
- Keyboard missing device is debug-only (not a warning)

## [2.0.0] - 2026-06-21

### Added

- **Unified 8-line viewport** — single scrolling terminal window (no output/input split)
- **5×8 font** — true 8-row glyphs, 21 cols × 8 lines on 128×64
- **Virtual scrollback** — pan with **Shift+Ctrl+Up/Down** (vertical) and **Ctrl+Left/Right** (horizontal)
- **Keyboard navigation** — evdev reader on tty1 for viewport pan keys
- **Prompt styling** — bold prompt prefix; reads Linux vcsa green color attributes
- **Dual-color OLED support** — top 16px yellow / bottom 48px blue (hardware zones)
- Per-row render diff for lower I2C traffic

### Removed

- Dual-pane layout (6+1 rows)
- Display rotation (`--rotate`) and `rotate.c`
- Unused `font4x6`, `font5x7`, `wrap.c`

### Changed

- `wrap.c` replaced by `viewport.c`
- `oledtty-ctl` kept as SSH fallback for pan commands

## [1.2.0] - 2026-06-21

### Fixed

- **Horizontal text by default** — rotation default is now `0` (landscape mount); removed broken `--rotate 90` from systemd service
- **Readable font** — restored 5×7 font (21 cols × 7 lines) replacing unreadable 4×6
- **Pi Zero responsiveness** — lower CPU priority (`nice 10`), history dedup, 80 ms poll interval
- Warning logged if `--rotate 90` or `270` is used (clips on 128×64)

### Changed

- Dual-pane layout: 6 output lines + 1 prompt line (horizontal)
- Version bump for daily-use release

## [1.1.0] - 2026-06-21

### Added

- Compact 4×6 font — 8 output lines + 1 input line (31 columns)
- Dual-pane layout: output area + fixed prompt line
- Display rotation (`--rotate 0/90/180/270`, default 90)
- Line history ring buffer for browse mode
- `oledtty-ctl` tool: up/down/left/right/live
- Smart path trimming for long lines in live mode

## [1.0.0] - 2026-06-21

### Added

- Real-time Linux console mirror via `/dev/vcsa1` (tty1)
- Direct SSD1306 I2C driver (no third-party OLED libraries)
- Embedded 5×7 font — 8 lines × 21 characters on 128×64 panels
- Cursor-following viewport with vertical and horizontal scroll
- Blinking block cursor
- Partial I2C page updates (dirty-region flush)
- systemd service with auto-restart
- `--dry-run`, `--once`, and other CLI options
- Install/uninstall scripts for Raspberry Pi deployment
