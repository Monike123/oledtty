#!/bin/bash
#
# install.sh — install and START oledtty on THIS Pi (OLED must be connected here).
#
# Pi 3 SD-card prep for Pi Zero: use setup-for-pizero.sh instead.
#
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_NAME="oledtty"
SERVICE_NAME="oledtty.service"

if [[ "$(id -u)" -ne 0 ]]; then
    exec sudo "$0" "$@"
fi

echo "[0/5] Fixing Windows CRLF (pendrive copy)"
chmod +x "$PROJECT_ROOT/fix-line-endings.sh" 2>/dev/null || true
if [[ -x "$PROJECT_ROOT/fix-line-endings.sh" ]]; then
    "$PROJECT_ROOT/fix-line-endings.sh"
else
    find "$PROJECT_ROOT" -type f \( -name '*.sh' -o -name '*.c' -o -name '*.h' \
        -o -name 'Makefile' -o -name '*.service' \) \
        ! -path '*/.git/*' -exec sed -i 's/\r$//' {} +
fi

echo "[1/5] Installing build dependencies"
apt-get update -qq
apt-get install -y build-essential i2c-tools libi2c-dev

echo "[2/5] Building project"
cd "$PROJECT_ROOT"
chmod +x setup-for-pizero.sh zero-start.sh install.sh uninstall.sh fix-line-endings.sh 2>/dev/null || true
make clean
make

echo "[3/5] Installing binaries"
install -m 0755 "$PROJECT_ROOT/build/$BIN_NAME" "/usr/local/bin/$BIN_NAME"
install -m 0755 "$PROJECT_ROOT/build/oledtty-ctl" "/usr/local/bin/oledtty-ctl"

echo "[4/5] Installing systemd service"
install -m 0644 "$PROJECT_ROOT/$SERVICE_NAME" "/etc/systemd/system/$SERVICE_NAME"
systemctl daemon-reload

echo "[5/5] Enabling and starting service"
systemctl disable --now oled.service 2>/dev/null || true
systemctl enable "$SERVICE_NAME"
systemctl restart "$SERVICE_NAME"

echo "Done."
echo "  systemctl status $SERVICE_NAME --no-pager"
echo "  journalctl -u $SERVICE_NAME -n 20 --no-pager"
echo "  which $BIN_NAME"
