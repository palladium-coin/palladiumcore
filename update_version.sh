#!/bin/bash
set -e

echo "========================================================"
echo "   Palladium Core - Master Version Updater"
echo "========================================================"

# 1. Get Inputs
read -p "Enter CURRENT version string (e.g. 1.4.3): " OLD_VERSION
read -p "Enter NEW version string (e.g. 1.5.0): " NEW_VERSION

if [ -z "$OLD_VERSION" ] || [ -z "$NEW_VERSION" ]; then
    echo "Error: Version strings cannot be empty."
    exit 1
fi

# Parse the NEW version into parts (Major.Minor.Revision)
IFS='.' read -r -a V_PARTS <<< "$NEW_VERSION"
MAJOR="${V_PARTS[0]}"
MINOR="${V_PARTS[1]}"
REV="${V_PARTS[2]}"

# Default to 0 if revision is missing (e.g. 1.5 becomes 1.5.0)
if [ -z "$REV" ]; then REV="0"; fi

echo "--------------------------------------------------------"
echo "Targeting: $MAJOR.$MINOR.$REV"
echo "--------------------------------------------------------"

# 2. Global Text Replacement (Docs, Manpages, Comments)
echo "[1/3] Performing global search & replace ($OLD_VERSION -> $NEW_VERSION)..."
# We exclude .git directory and binaries to avoid corruption
grep -rIl "$OLD_VERSION" . | grep -v "^./.git" | grep -v "update_version.sh" | while read -r file ; do
    sed -i "s/$OLD_VERSION/$NEW_VERSION/g" "$file"
done
echo "Global text replacement done."

# 3. Update configure.ac (The Linux/Unix Build System)
echo "[2/3] Updating configure.ac..."
if [ -f "configure.ac" ]; then
    sed -i "s/define(_CLIENT_VERSION_MAJOR, [0-9]*)/define(_CLIENT_VERSION_MAJOR, $MAJOR)/g" configure.ac
    sed -i "s/define(_CLIENT_VERSION_MINOR, [0-9]*)/define(_CLIENT_VERSION_MINOR, $MINOR)/g" configure.ac
    sed -i "s/define(_CLIENT_VERSION_REVISION, [0-9]*)/define(_CLIENT_VERSION_REVISION, $REV)/g" configure.ac
    echo "✔ configure.ac updated."
else
    echo "❌ Warning: configure.ac not found."
fi

# 4. Update Windows MSVC Config (The Windows Build System)
echo "[3/3] Updating build_msvc/palladium_config.h..."
MSVC_FILE="build_msvc/palladium_config.h"
if [ -f "$MSVC_FILE" ]; then
    sed -i "s/#define CLIENT_VERSION_MAJOR [0-9]*/#define CLIENT_VERSION_MAJOR $MAJOR/g" "$MSVC_FILE"
    sed -i "s/#define CLIENT_VERSION_MINOR [0-9]*/#define CLIENT_VERSION_MINOR $MINOR/g" "$MSVC_FILE"
    sed -i "s/#define CLIENT_VERSION_REVISION [0-9]*/#define CLIENT_VERSION_REVISION $REV/g" "$MSVC_FILE"
    echo "✔ $MSVC_FILE updated."
else
    echo "❌ Warning: $MSVC_FILE not found."
fi

echo "========================================================"
echo "Update Complete! Please run:"
echo "1. make clean"
echo "2. ./autogen.sh"
echo "3. ./configure"
echo "4. make"
echo "========================================================"
