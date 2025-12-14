#!/usr/bin/env bash
set -euo pipefail

# Directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Function to run a build script
run_build() {
    local script_name="$1"
    local description="$2"
    shift 2
    
    echo "============================================================"
    echo "Starting build for: $description"
    echo "Script: $script_name $*"
    echo "============================================================"
    
    if [[ -f "${SCRIPT_DIR}/${script_name}" ]]; then
        # Ensure it's executable
        chmod +x "${SCRIPT_DIR}/${script_name}"
        "${SCRIPT_DIR}/${script_name}" "$@"
    else
        echo "Error: Script ${script_name} not found!"
    fi
    
    echo "============================================================"
    echo "Finished build for: $description"
    echo "============================================================"
    echo ""
}

show_menu() {
    echo "Palladium Core Docker Build System"
    echo "----------------------------------"
    echo "1) Linux x86_64"
    echo "2) Linux aarch64 (ARM64)"
    echo "3) Linux armv7l (ARM 32-bit)"
    echo "4) Windows x86_64"
    echo "4a) Windows x86_64 (Installer)"
    echo "5) Build ALL (Binaries and Installers)"
    echo "6) Build ALL Installers"
    echo "0) Exit"
    echo ""
}

show_menu
read -p "Enter your choice(s) separated by space (e.g. '1 4a' for Linux and Windows Installer): " choices

# Convert string to array
read -ra ADDR <<< "$choices"

# Check if '5' (ALL) or '6' is selected
do_all=false
do_all_installers=false
for choice in "${ADDR[@]}"; do
    if [[ "$choice" == "5" ]]; then
        do_all=true
        break
    fi
    if [[ "$choice" == "6" ]]; then
        do_all_installers=true
        break
    fi
done

if [ "$do_all" = true ]; then
    run_build "build-linux-x86_64.sh" "Linux x86_64"
    run_build "build-linux-aarch64.sh" "Linux aarch64"
    run_build "build-linux-armv7l.sh" "Linux armv7l"
    run_build "build-windows.sh" "Windows x86_64"
    run_build "build-windows.sh" "Windows x86_64 (Installer)" --installer
elif [ "$do_all_installers" = true ]; then
    run_build "build-windows.sh" "Windows x86_64 (Installer)" --installer
else
    for choice in "${ADDR[@]}"; do
        case "$choice" in
            1)
                run_build "build-linux-x86_64.sh" "Linux x86_64"
                ;;
            2)
                run_build "build-linux-aarch64.sh" "Linux aarch64"
                ;;
            3)
                run_build "build-linux-armv7l.sh" "Linux armv7l"
                ;;
            4)
                run_build "build-windows.sh" "Windows x86_64"
                ;;
            "4a")
                run_build "build-windows.sh" "Windows x86_64 (Installer)" --installer
                ;;
            0)
                echo "Exiting."
                exit 0
                ;;
            *)
                echo "Invalid option: $choice"
                ;;
        esac
    done
fi

echo "All selected builds completed."
