#!/usr/bin/env python3
"""Generate a synthetic /dev/vcsa snapshot for offline oledtty tests."""

from __future__ import annotations

import struct
import sys
from pathlib import Path

ROWS = 30
COLS = 80


def blank_screen() -> list[list[tuple[int, int]]]:
    return [[(ord(" "), 0x07) for _ in range(COLS)] for _ in range(ROWS)]


def put_line(screen: list[list[tuple[int, int]]], row: int, text: str) -> None:
    for col, ch in enumerate(text[:COLS]):
        screen[row][col] = (ord(ch), 0x07)


def build_screen() -> list[list[tuple[int, int]]]:
    screen = blank_screen()
    lines = [
        "Debian GNU/Linux 12 kali tty1",
        "",
        "kali login: root",
        "Password:",
        "",
        "Linux kali 6.1.0-rpi-rpi-v6 #1 SMP PREEMPT",
        "",
        "kali@kali:~$ ls",
        "oledtty  README.md  setup-for-pizero.sh",
        "kali@kali:~$ sudo ./build/oledtty --dry-run --once",
        "kali@kali:~$ _",
    ]

    for idx, line in enumerate(lines):
        put_line(screen, idx, line)

    return screen


def cursor_position(screen: list[list[tuple[int, int]]], prompt_row: int) -> tuple[int, int]:
    for row in range(ROWS):
        line = "".join(chr(screen[row][col][0]) for col in range(COLS)).rstrip()
        if line.endswith("_"):
            col = len(line) - 1
            screen[row][col] = (ord(" "), 0x07)
            return col, row
    return 0, prompt_row


def write_vcsa(path: Path) -> None:
    screen = build_screen()
    cursor_x, cursor_y = cursor_position(screen, 10)

    payload = bytearray()
    payload.extend(struct.pack("BBBB", ROWS, COLS, cursor_x, cursor_y))

    for row in range(ROWS):
        for col in range(COLS):
            ch, attr = screen[row][col]
            payload.extend(struct.pack("BB", ch, attr))

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)
    print(f"wrote {path} ({len(payload)} bytes, cursor=({cursor_x},{cursor_y}))")


def main() -> int:
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "tests/sample.vcsa")
    write_vcsa(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
