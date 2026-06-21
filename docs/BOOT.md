# Boot Integration

The installer places the unit file at:

```text
/etc/systemd/system/oledtty.service
```

## Commands

```bash
sudo systemctl enable oledtty
sudo systemctl start oledtty
sudo systemctl status oledtty
journalctl -u oledtty -f
```

## Service details

- Runs as **root** (required to read `/dev/vcsa1`)
- Starts after `multi-user.target`
- Restarts automatically after 2 seconds on failure

## Force a full redraw

Useful after a VT switch or if the display looks stale:

```bash
sudo kill -USR1 $(pidof oledtty)
```

## Disable at boot

```bash
sudo systemctl disable oledtty
sudo systemctl stop oledtty
```
