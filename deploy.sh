#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PIO="${PIO:-$(command -v pio 2>/dev/null || echo "$HOME/Library/Python/3.9/bin/pio")}"

if [ ! -x "$PIO" ]; then
    echo "Error: PlatformIO not found. Install it or set PIO=/path/to/pio"
    exit 1
fi

# Check secrets.h exists
if [ ! -f "$SCRIPT_DIR/src/secrets.h" ]; then
    echo "Error: src/secrets.h not found."
    echo "  cp src/secrets.h.example src/secrets.h"
    echo "  Then fill in your WiFi and HA credentials."
    exit 1
fi

usage() {
    echo "Usage: $0 [build|flash|monitor|all]"
    echo "  build   - Compile firmware"
    echo "  flash   - Build and upload to board"
    echo "  monitor - Open serial monitor (115200 baud)"
    echo "  all     - Build, flash, and open monitor"
    echo ""
    echo "Default: flash"
}

cmd_build() {
    echo "==> Building..."
    "$PIO" run -d "$SCRIPT_DIR"
    echo "==> Build OK"
}

cmd_flash() {
    echo "==> Building and flashing..."
    "$PIO" run -d "$SCRIPT_DIR" -t upload
    echo "==> Flash OK"
}

cmd_monitor() {
    echo "==> Opening serial monitor (Ctrl+C to exit)..."
    "$PIO" device monitor -d "$SCRIPT_DIR" -b 115200
}

ACTION="${1:-flash}"

case "$ACTION" in
    build)   cmd_build ;;
    flash)   cmd_flash ;;
    monitor) cmd_monitor ;;
    all)     cmd_flash; cmd_monitor ;;
    -h|--help|help) usage ;;
    *)
        echo "Unknown action: $ACTION"
        usage
        exit 1
        ;;
esac
