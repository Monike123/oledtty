#!/bin/bash
# fix-line-endings.sh — strip Windows CRLF after copying oledtty from a PC pendrive.
# Run once on the Pi before build:  ./fix-line-endings.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

fix_file() {
    sed -i 's/\r$//' "$1"
}

count=0
while IFS= read -r -d '' f; do
    fix_file "$f"
    count=$((count + 1))
done < <(find . -type f \( \
    -name '*.sh' -o -name '*.c' -o -name '*.h' -o \
    -name 'Makefile' -o -name '*.service' -o -name '.gitattributes' \
\) ! -path './.git/*' -print0)

echo "Fixed line endings in $count files under $ROOT"
