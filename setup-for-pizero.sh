#!/bin/bash
#
# setup-for-pizero.sh — prepare SD card on Pi 3/4, then move to Pi Zero 1.3.
#
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_NAME="oledtty"
SERVICE_NAME="oledtty.service"

if [[ "$(id -u)" -ne 0 ]]; then
    echo "Run as root:  sudo $0"
    exit 1
fi

echo "=============================================="
echo "  oledtty — prepare SD card for Pi Zero 1.3"
echo "=============================================="

fix_windows_line_endings() {
    echo "[0/5] Fixing Windows CRLF (pendrive copy)"
    chmod +x "$PROJECT_ROOT/fix-line-endings.sh" 2>/dev/null || true
    if [[ -x "$PROJECT_ROOT/fix-line-endings.sh" ]]; then
        "$PROJECT_ROOT/fix-line-endings.sh"
    else
        find "$PROJECT_ROOT" -type f \( -name '*.sh' -o -name '*.c' -o -name '*.h' \
            -o -name 'Makefile' -o -name '*.service' \) \
            ! -path '*/.git/*' -exec sed -i 's/\r$//' {} +
    fi
}

enable_i2c_in_config() {
    echo "[1/5] Enabling I2C in boot config"
    local patched=0
    for cfg in /boot/firmware/config.txt /boot/config.txt; do
        if [[ -f "$cfg" ]]; then
            echo "       editing $cfg"
            if grep -q '^#dtparam=i2c_arm=on' "$cfg" 2>/dev/null; then
                sed -i 's/^#dtparam=i2c_arm=on/dtparam=i2c_arm=on/' "$cfg"
            fi
            if ! grep -q '^dtparam=i2c_arm=on' "$cfg" 2>/dev/null; then
                echo 'dtparam=i2c_arm=on' >> "$cfg"
            fi
            if ! grep -q '^dtparam=i2c_arm_baudrate=' "$cfg" 2>/dev/null; then
                echo 'dtparam=i2c_arm_baudrate=400000' >> "$cfg"
            fi
            patched=1
        fi
    done
    if [[ "$patched" -eq 0 ]]; then
        echo "WARNING: config.txt not found — enable I2C manually"
    fi
}

quiet_console_cmdline() {
    echo "[2/5] Quieting kernel messages on tty1"
    for cmdline in /boot/firmware/cmdline.txt /boot/cmdline.txt; do
        if [[ -f "$cmdline" ]]; then
            echo "       editing $cmdline"
            local line
            line="$(tr -d '\n' < "$cmdline")"
            if ! grep -q 'console=tty1' <<< "$line"; then
                line="$line console=tty1"
            fi
            if ! grep -q 'loglevel=3' <<< "$line"; then
                line="$line loglevel=3"
            fi
            if ! grep -q ' quiet' <<< "$line"; then
                line="$line quiet"
            fi
            printf '%s\n' "$line" > "$cmdline"
        fi
    done
}

install_build_deps() {
    echo "[3/5] Installing build tools"
    apt-get update -qq
    apt-get install -y build-essential libi2c-dev file
}

build_for_pizero() {
    echo "[4/5] Building oledtty for Pi Zero (ARMv6)"
    cd "$PROJECT_ROOT"
    chmod +x setup-for-pizero.sh fix-line-endings.sh zero-start.sh install.sh uninstall.sh 2>/dev/null || true
    make clean
    make

    if command -v file >/dev/null 2>&1; then
        local info
        info=$(file "$PROJECT_ROOT/build/$BIN_NAME")
        echo "       $info"
        if echo "$info" | grep -qE 'v7|aarch64'; then
            echo "ERROR: binary is not ARMv6 — will not run on Pi Zero 1.3"
            exit 1
        fi
    fi
}

install_files() {
    install -m 0755 "$PROJECT_ROOT/build/$BIN_NAME" "/usr/local/bin/$BIN_NAME"
    install -m 0755 "$PROJECT_ROOT/build/oledtty-ctl" "/usr/local/bin/oledtty-ctl"
    install -m 0644 "$PROJECT_ROOT/$SERVICE_NAME" "/etc/systemd/system/$SERVICE_NAME"
    install -d /usr/local/share/doc/oledtty
    [[ -f "$PROJECT_ROOT/README.md" ]] && \
        install -m 0644 "$PROJECT_ROOT/README.md" /usr/local/share/doc/oledtty/

    systemctl daemon-reload
    systemctl enable "$SERVICE_NAME"
    systemctl disable --now oled.service 2>/dev/null || true
    systemctl stop "$SERVICE_NAME" 2>/dev/null || true
}

print_done() {
    echo "[5/5] Done"
    echo ""
    echo "  sudo shutdown -h now"
    echo "  Move SD card to Pi Zero 1.3, power on."
    echo ""
    echo "  On Pi Zero if needed:"
    echo "    sudo systemctl restart oledtty"
    echo "    journalctl -u oledtty -n 20 --no-pager"
}

enable_i2c_in_config
quiet_console_cmdline
install_build_deps
fix_windows_line_endings
build_for_pizero
install_files
print_done
