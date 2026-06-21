#!/bin/bash
#
# zero-start.sh — restart oledtty on the Pi Zero if needed.
#
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
    exec sudo "$0" "$@"
fi

systemctl daemon-reload
systemctl disable --now oled.service 2>/dev/null || true
systemctl enable oledtty.service
systemctl restart oledtty.service
systemctl status oledtty.service --no-pager || true
echo ""
echo "Logs: journalctl -u oledtty -f"
