#!/bin/bash
set -euo pipefail

SERVICE_NAME="oledtty.service"
BIN_NAME="oledtty"

systemctl stop "$SERVICE_NAME" 2>/dev/null || true
systemctl disable "$SERVICE_NAME" 2>/dev/null || true
rm -f "/etc/systemd/system/$SERVICE_NAME"
rm -f "/usr/local/bin/$BIN_NAME"
rm -f "/usr/local/bin/oledtty-ctl"
systemctl daemon-reload

echo "Uninstalled."
