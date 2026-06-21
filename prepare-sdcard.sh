#!/bin/bash
#
# prepare-sdcard.sh
# -----------------
# Run ONCE on a Raspberry Pi 3/4/5 (with internet) to prepare an SD card
# for a headless Pi Zero 1.3 with no WiFi.
#
# This script:
#   - Builds oledtty for ARMv6 (Pi Zero compatible)
#   - Enables I2C in the SD card boot config
#   - Installs the binary + systemd service ON THE SD CARD
#   - Enables the service so the Pi Zero starts the OLED mirror on boot
#
# It does NOT run i2cdetect or start oledtty on the Pi 3 — the OLED is
# not connected there. Everything is written to the SD card for the Zero.
#
# Usage:
#   sudo ./prepare-sdcard.sh --root /mnt/rootfs --boot /mnt/bootfs
#   sudo ./prepare-sdcard.sh              # try auto-detect mounts
#
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_NAME="oledtty"
SERVICE_NAME="oledtty.service"

ROOT_MNT=""
BOOT_MNT=""
SKIP_BUILD=0

usage() {
    cat <<EOF
Usage: sudo $0 [options]

Prepare an SD card (Pi Zero target) using a Pi 3/4/5 as build machine.

Options:
  --root PATH   Mount point of the SD card root partition (ext4)
  --boot PATH   Mount point of the SD card boot partition (FAT)
  --skip-build  Skip compile (use existing build/oledtty)
  -h, --help    Show this help

Examples:
  sudo $0 --root /media/user/rootfs --boot /media/user/bootfs
  sudo $0

After success: safely eject the SD card, insert into Pi Zero, power on.
No commands needed on the Pi Zero — the OLED mirror starts automatically.

EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --root) ROOT_MNT="$2"; shift 2 ;;
        --boot) BOOT_MNT="$2"; shift 2 ;;
        --skip-build) SKIP_BUILD=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown option: $1"; usage; exit 1 ;;
    esac
done

if [[ "$(id -u)" -ne 0 ]]; then
    echo "ERROR: run as root: sudo $0"
    exit 1
fi

try_auto_detect() {
    echo "[detect] Looking for mounted SD card partitions..."

    local candidates_root candidates_boot
    candidates_root=$(ls -d /media/*/rootfs /media/*/*/rootfs /mnt/rootfs 2>/dev/null || true)
    candidates_boot=$(ls -d /media/*/bootfs /media/*/boot /media/*/*/bootfs /media/*/*/boot /mnt/boot 2>/dev/null || true)

    if [[ -z "$ROOT_MNT" && -n "$candidates_root" ]]; then
        ROOT_MNT=$(echo "$candidates_root" | head -1)
        echo "[detect] root partition: $ROOT_MNT"
    fi
    if [[ -z "$BOOT_MNT" && -n "$candidates_boot" ]]; then
        BOOT_MNT=$(echo "$candidates_boot" | head -1)
        echo "[detect] boot partition: $BOOT_MNT"
    fi
}

validate_mounts() {
    if [[ -z "$ROOT_MNT" || ! -d "$ROOT_MNT/etc" ]]; then
        echo "ERROR: SD card root partition not found."
        echo "       Mount the SD card root (ext4) and pass: --root /path/to/rootfs"
        exit 1
    fi
    if [[ -z "$BOOT_MNT" || ! -d "$BOOT_MNT" ]]; then
        echo "ERROR: SD card boot partition not found."
        echo "       Mount the SD card boot (FAT) and pass: --boot /path/to/bootfs"
        exit 1
    fi
    echo "[ok] root: $ROOT_MNT"
    echo "[ok] boot: $BOOT_MNT"
}

install_build_deps() {
    echo "[1/6] Installing build tools on THIS Pi (Pi 3/4/5 — not on the SD card OS)"
    if command -v apt-get >/dev/null 2>&1; then
        apt-get update -qq
        apt-get install -y build-essential libi2c-dev file
    else
        echo "ERROR: apt-get not found. Install gcc and libi2c-dev manually."
        exit 1
    fi
}

build_for_pizero() {
    echo "[2/6] Building oledtty for Pi Zero (ARMv6)"
    cd "$PROJECT_ROOT"
    chmod +x prepare-sdcard.sh install.sh uninstall.sh 2>/dev/null || true
    if [[ "$SKIP_BUILD" -eq 0 ]]; then
        make clean
        make
    elif [[ ! -x "$PROJECT_ROOT/build/$BIN_NAME" ]]; then
        echo "ERROR: build/$BIN_NAME not found. Remove --skip-build or run make first."
        exit 1
    fi

    if command -v file >/dev/null 2>&1; then
        local info
        info=$(file "$PROJECT_ROOT/build/$BIN_NAME")
        echo "       $info"
        if echo "$info" | grep -qE 'x86-64|Intel 80386'; then
            echo "ERROR: built for wrong architecture"
            exit 1
        fi
        if echo "$info" | grep -q 'ARM' && echo "$info" | grep -qE 'v7|aarch64'; then
            echo "ERROR: binary is ARMv7+/64 — will not run on Pi Zero 1.3"
            echo "       The Makefile should use -march=armv6. Check your build."
            exit 1
        fi
    fi
}

enable_i2c_on_boot_partition() {
    echo "[3/6] Enabling I2C in SD card boot config"

    local config_files=()
    [[ -f "$BOOT_MNT/config.txt" ]] && config_files+=("$BOOT_MNT/config.txt")
    [[ -f "$BOOT_MNT/firmware/config.txt" ]] && config_files+=("$BOOT_MNT/firmware/config.txt")

    if [[ ${#config_files[@]} -eq 0 ]]; then
        echo "ERROR: config.txt not found under $BOOT_MNT"
        exit 1
    fi

    for cfg in "${config_files[@]}"; do
        echo "       patching $cfg"
        if ! grep -q '^dtparam=i2c_arm=on' "$cfg" 2>/dev/null; then
            echo 'dtparam=i2c_arm=on' >> "$cfg"
        fi
        if ! grep -q '^dtparam=i2c_arm_baudrate=' "$cfg" 2>/dev/null; then
            echo 'dtparam=i2c_arm_baudrate=400000' >> "$cfg"
        fi
        if grep -q '^#dtparam=i2c_arm=on' "$cfg" 2>/dev/null; then
            sed -i 's/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/' "$cfg"
        fi
    done
}

install_to_sdcard() {
    echo "[4/6] Installing binary to SD card: $ROOT_MNT/usr/local/bin/$BIN_NAME"
    install -d "$ROOT_MNT/usr/local/bin"
    install -m 0755 "$PROJECT_ROOT/build/$BIN_NAME" "$ROOT_MNT/usr/local/bin/$BIN_NAME"
    install -m 0755 "$PROJECT_ROOT/build/oledtty-ctl" "$ROOT_MNT/usr/local/bin/oledtty-ctl"

    echo "[5/6] Installing systemd service on SD card"
    install -d "$ROOT_MNT/etc/systemd/system"
    install -m 0644 "$PROJECT_ROOT/$SERVICE_NAME" "$ROOT_MNT/etc/systemd/system/$SERVICE_NAME"

    install -d "$ROOT_MNT/etc/systemd/system/multi-user.target.wants"
    ln -sf "../$SERVICE_NAME" \
        "$ROOT_MNT/etc/systemd/system/multi-user.target.wants/$SERVICE_NAME"

    install -d "$ROOT_MNT/usr/local/share/doc/oledtty"
    [[ -f "$PROJECT_ROOT/README.md" ]] && \
        install -m 0644 "$PROJECT_ROOT/README.md" "$ROOT_MNT/usr/local/share/doc/oledtty/"
}

print_done() {
    echo "[6/6] Done — SD card is ready for Pi Zero 1.3"
    echo ""
    echo "=========================================="
    echo "  NEXT STEPS (no WiFi on Pi Zero needed)"
    echo "=========================================="
    echo ""
    echo "  1. Safely eject the SD card from this Pi 3"
    echo "  2. Insert SD card into Pi Zero 1.3"
    echo "  3. Connect the I2C OLED (VCC/GND/SCL/SDA)"
    echo "  4. Power on — NO commands needed on the Pi Zero"
    echo ""
    echo "  The OLED should show boot text, then the login prompt."
    echo "  Service: oledtty (enabled, runs as root on boot)"
    echo ""
    echo "  If you ever need to restart the display manually on the Zero:"
    echo "    sudo systemctl restart oledtty"
    echo ""
    echo "  NOTE: i2cdetect was NOT run here — the OLED is on the Zero,"
    echo "        not on this Pi 3. That is expected and correct."
    echo ""
}

try_auto_detect
validate_mounts
install_build_deps
build_for_pizero
enable_i2c_on_boot_partition
install_to_sdcard
print_done
