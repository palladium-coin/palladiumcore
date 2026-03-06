#!/usr/bin/env bash
export LC_ALL=C
set -e

echo "========================================================"
echo "   Palladium Core - Master Version Updater"
echo "========================================================"

CONFIGURE_FILE="configure.ac"
MSVC_FILE="build_msvc/palladium_config.h"
MANPAGES=(
    "doc/man/palladium-cli.1"
    "doc/man/palladium-qt.1"
    "doc/man/palladium-tx.1"
    "doc/man/palladium-wallet.1"
    "doc/man/palladiumd.1"
)

# 1. Detect current version and ask only for the target one.
if [ ! -f "$CONFIGURE_FILE" ]; then
    echo "Error: $CONFIGURE_FILE not found."
    exit 1
fi

CURRENT_MAJOR=$(sed -n 's/^define(_CLIENT_VERSION_MAJOR, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)
CURRENT_MINOR=$(sed -n 's/^define(_CLIENT_VERSION_MINOR, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)
CURRENT_REV=$(sed -n 's/^define(_CLIENT_VERSION_REVISION, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)

if [ -z "$CURRENT_MAJOR" ] || [ -z "$CURRENT_MINOR" ] || [ -z "$CURRENT_REV" ]; then
    echo "Error: could not detect current version from $CONFIGURE_FILE."
    exit 1
fi

OLD_VERSION="${CURRENT_MAJOR}.${CURRENT_MINOR}.${CURRENT_REV}"
echo "Current version detected: $OLD_VERSION"
read -p "Enter NEW version string (e.g. 1.5.0): " NEW_VERSION

if [ -z "$NEW_VERSION" ]; then
    echo "Error: NEW version string cannot be empty."
    exit 1
fi

# Parse the NEW version into parts (Major.Minor.Revision)
IFS='.' read -r -a V_PARTS <<< "$NEW_VERSION"
MAJOR="${V_PARTS[0]}"
MINOR="${V_PARTS[1]}"
REV="${V_PARTS[2]}"

# Default to 0 if revision is missing (e.g. 1.5 becomes 1.5.0)
if [ -z "$REV" ]; then REV="0"; fi
if [ -z "$MAJOR" ] || [ -z "$MINOR" ]; then
    echo "Error: NEW version must be in MAJOR.MINOR[.REVISION] format."
    exit 1
fi

echo "--------------------------------------------------------"
echo "Targeting: $MAJOR.$MINOR.$REV"
echo "--------------------------------------------------------"

TARGET_FILES=("$CONFIGURE_FILE" "$MSVC_FILE" "${MANPAGES[@]}")
OLD_VERSION_REGEX=$(printf '%s' "$OLD_VERSION" | sed 's/[][\/.^$*+?|(){}-]/\\&/g')
NEW_VERSION_REPL=$(printf '%s' "$NEW_VERSION" | sed 's/[&|]/\\&/g')

echo "[1/4] Validating target file whitelist..."
MISSING_FILES=()
for file in "${TARGET_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        MISSING_FILES+=("$file")
    fi
done
if [ "${#MISSING_FILES[@]}" -ne 0 ]; then
    echo "Error: required target files are missing:"
    for file in "${MISSING_FILES[@]}"; do
        echo "  - $file"
    done
    exit 1
fi
echo "✔ All target files are present."

# 2. Update configure.ac (Linux/Unix build metadata)
echo "[2/4] Updating $CONFIGURE_FILE..."
sed -i "s/define(_CLIENT_VERSION_MAJOR, [0-9]*)/define(_CLIENT_VERSION_MAJOR, $MAJOR)/g" "$CONFIGURE_FILE"
sed -i "s/define(_CLIENT_VERSION_MINOR, [0-9]*)/define(_CLIENT_VERSION_MINOR, $MINOR)/g" "$CONFIGURE_FILE"
sed -i "s/define(_CLIENT_VERSION_REVISION, [0-9]*)/define(_CLIENT_VERSION_REVISION, $REV)/g" "$CONFIGURE_FILE"
echo "✔ $CONFIGURE_FILE updated."

# 3. Update Windows MSVC config
echo "[3/4] Updating $MSVC_FILE..."
sed -i "s/#define CLIENT_VERSION_MAJOR [0-9]*/#define CLIENT_VERSION_MAJOR $MAJOR/g" "$MSVC_FILE"
sed -i "s/#define CLIENT_VERSION_MINOR [0-9]*/#define CLIENT_VERSION_MINOR $MINOR/g" "$MSVC_FILE"
sed -i "s/#define CLIENT_VERSION_REVISION [0-9]*/#define CLIENT_VERSION_REVISION $REV/g" "$MSVC_FILE"
echo "✔ $MSVC_FILE updated."

# 4. Update the five generated manpages via deterministic text replacement
echo "[4/4] Updating manpages ($OLD_VERSION -> $NEW_VERSION)..."
for file in "${MANPAGES[@]}"; do
    sed -i "s|$OLD_VERSION_REGEX|$NEW_VERSION_REPL|g" "$file"
done
echo "✔ Manpages updated."

echo "========================================================"
echo "Update complete. Updated files:"
for file in "${TARGET_FILES[@]}"; do
    echo " - $file"
done
echo "========================================================"
