#!/usr/bin/env bash
# Interactive menu for the reproducible Docker builds. Same as calling build.sh and
# check-reproducible.sh directly (see README.md).
export LC_ALL=C
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

run() {
    echo "============================================================"
    echo "$*"
    echo "============================================================"
    "$@"
    echo
}

show_menu() {
    cat <<'MENU'
Palladium Core Docker Build System
----------------------------------
 1) Linux x86_64
 2) Linux aarch64 (ARM64)
 3) Linux armv7l (ARM 32-bit)
 4) Windows x86_64
4a) Windows x86_64 + installer
 5) All binaries (1-4)
 6) All binaries and the Windows installer
 7) Check reproducibility of one target (builds it twice)
 0) Exit

MENU
}

show_menu
read -rp "Choice(s), separated by space (e.g. '1 4a'): " -a choices

for choice in "${choices[@]}"; do
    case "${choice}" in
    1) run "${SCRIPT_DIR}/build.sh" linux-x86_64 ;;
    2) run "${SCRIPT_DIR}/build.sh" linux-aarch64 ;;
    3) run "${SCRIPT_DIR}/build.sh" linux-armv7l ;;
    4) run "${SCRIPT_DIR}/build.sh" windows ;;
    4a) run "${SCRIPT_DIR}/build.sh" windows --installer ;;
    5)
        for t in linux-x86_64 linux-aarch64 linux-armv7l windows; do
            run "${SCRIPT_DIR}/build.sh" "${t}"
        done
        ;;
    6)
        for t in linux-x86_64 linux-aarch64 linux-armv7l; do
            run "${SCRIPT_DIR}/build.sh" "${t}"
        done
        run "${SCRIPT_DIR}/build.sh" windows --installer
        ;;
    7)
        read -rp "Target (linux-x86_64, linux-aarch64, linux-armv7l, windows): " target
        run "${SCRIPT_DIR}/check-reproducible.sh" "${target}"
        ;;
    0)
        echo "Exiting."
        exit 0
        ;;
    *) echo "Invalid option: ${choice}" ;;
    esac
done
echo "All selected builds completed."
