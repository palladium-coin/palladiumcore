#!/usr/bin/env bash
# Sets the version of Palladium Core in every place that holds it.
#
#   ./update_version.sh [--rc N] [--dev] [MAJOR.MINOR[.REVISION]]
#
#   --rc N   release candidate number (CLIENT_VERSION_RC); default 0, a final release
#   --dev    set CLIENT_VERSION_IS_RELEASE to false (a development version); default true
#
# Without the version it asks for it. Run it from the root of the repository.
export LC_ALL=C
set -euo pipefail

CONFIGURE_FILE="configure.ac"
MSVC_FILE="build_msvc/palladium_config.h"
MANPAGES=(
    "doc/man/palladium-cli.1"
    "doc/man/palladium-qt.1"
    "doc/man/palladium-tx.1"
    "doc/man/palladium-wallet.1"
    "doc/man/palladiumd.1"
)

usage() {
    sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//' >&2
}

RC=0
IS_RELEASE=true
NEW_VERSION=""
while [ $# -gt 0 ]; do
    case "$1" in
    --rc)
        [ $# -ge 2 ] || { usage; exit 1; }
        RC="$2"
        shift 2
        ;;
    --dev)
        IS_RELEASE=false
        shift
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    -*)
        echo "Error: unknown option $1" >&2
        usage
        exit 1
        ;;
    *)
        [ -z "$NEW_VERSION" ] || { echo "Error: more than one version given." >&2; exit 1; }
        NEW_VERSION="${1#v}"
        shift
        ;;
    esac
done

if ! [[ "$RC" =~ ^[0-9]+$ ]]; then
    echo "Error: --rc needs a number." >&2
    exit 1
fi

echo "========================================================"
echo "   Palladium Core - Version Updater"
echo "========================================================"

TARGET_FILES=("$CONFIGURE_FILE" "$MSVC_FILE" "${MANPAGES[@]}")
echo "[1/5] Checking that the files to update are present..."
MISSING_FILES=()
for file in "${TARGET_FILES[@]}"; do
    [ -f "$file" ] || MISSING_FILES+=("$file")
done
if [ "${#MISSING_FILES[@]}" -ne 0 ]; then
    echo "Error: required files are missing (run it from the root of the repository):" >&2
    printf '  - %s\n' "${MISSING_FILES[@]}" >&2
    exit 1
fi

CURRENT_MAJOR=$(sed -n 's/^define(_CLIENT_VERSION_MAJOR, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)
CURRENT_MINOR=$(sed -n 's/^define(_CLIENT_VERSION_MINOR, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)
CURRENT_REV=$(sed -n 's/^define(_CLIENT_VERSION_REVISION, \([0-9][0-9]*\)).*/\1/p' "$CONFIGURE_FILE" | head -n1)
if [ -z "$CURRENT_MAJOR" ] || [ -z "$CURRENT_MINOR" ] || [ -z "$CURRENT_REV" ]; then
    echo "Error: could not detect the current version in $CONFIGURE_FILE." >&2
    exit 1
fi
OLD_VERSION="${CURRENT_MAJOR}.${CURRENT_MINOR}.${CURRENT_REV}"
echo "Current version: $OLD_VERSION"

if [ -z "$NEW_VERSION" ]; then
    read -r -p "Enter the NEW version (e.g. 2.1.1): " NEW_VERSION
    NEW_VERSION="${NEW_VERSION#v}"
fi

IFS='.' read -r MAJOR MINOR REV EXTRA <<< "$NEW_VERSION"
REV="${REV:-0}"
if [ -n "${EXTRA:-}" ] || ! [[ "$MAJOR" =~ ^[0-9]+$ && "$MINOR" =~ ^[0-9]+$ && "$REV" =~ ^[0-9]+$ ]]; then
    echo "Error: the version must be MAJOR.MINOR[.REVISION], with numbers only." >&2
    exit 1
fi
NEW_VERSION="${MAJOR}.${MINOR}.${REV}"

echo "--------------------------------------------------------"
echo "New version: $NEW_VERSION  (rc: $RC, release: $IS_RELEASE)"
echo "--------------------------------------------------------"

echo "[2/5] Updating $CONFIGURE_FILE..."
sed -i \
    -e "s/^define(_CLIENT_VERSION_MAJOR, [0-9]*)/define(_CLIENT_VERSION_MAJOR, $MAJOR)/" \
    -e "s/^define(_CLIENT_VERSION_MINOR, [0-9]*)/define(_CLIENT_VERSION_MINOR, $MINOR)/" \
    -e "s/^define(_CLIENT_VERSION_REVISION, [0-9]*)/define(_CLIENT_VERSION_REVISION, $REV)/" \
    -e "s/^define(_CLIENT_VERSION_RC, [0-9]*)/define(_CLIENT_VERSION_RC, $RC)/" \
    -e "s/^define(_CLIENT_VERSION_IS_RELEASE, [a-z]*)/define(_CLIENT_VERSION_IS_RELEASE, $IS_RELEASE)/" \
    "$CONFIGURE_FILE"

echo "[3/5] Updating $MSVC_FILE..."
sed -i \
    -e "s/^#define CLIENT_VERSION_MAJOR [0-9]*/#define CLIENT_VERSION_MAJOR $MAJOR/" \
    -e "s/^#define CLIENT_VERSION_MINOR [0-9]*/#define CLIENT_VERSION_MINOR $MINOR/" \
    -e "s/^#define CLIENT_VERSION_REVISION [0-9]*/#define CLIENT_VERSION_REVISION $REV/" \
    -e "s/^#define CLIENT_VERSION_IS_RELEASE [a-z]*/#define CLIENT_VERSION_IS_RELEASE $IS_RELEASE/" \
    "$MSVC_FILE"

# What the binaries print: 2.1.0 for a final release, 2.1.0rc1 for a candidate
FULL_VERSION="$NEW_VERSION"
[ "$RC" -eq 0 ] || FULL_VERSION="${NEW_VERSION}rc${RC}"

echo "[4/5] Updating the man pages (v$OLD_VERSION -> v$FULL_VERSION, date $(date +'%B %Y'))..."
OLD_REGEX="v$(printf '%s' "$OLD_VERSION" | sed 's/\./\\./g')"
MAN_DATE="$(date +'%B %Y')"
for file in "${MANPAGES[@]}"; do
    # The version only where help2man writes it (prefixed with "v"), the date in the .TH line
    sed -i \
        -e "s/${OLD_REGEX}\([^0-9.]\|\$\)/v${FULL_VERSION}\1/g" \
        -e "/^\.TH /s/\"[A-Za-z]* [0-9]\{4\}\"/\"${MAN_DATE}\"/" \
        "$file"
done

echo "[5/5] Checking the result..."
fail=0
grep -q "^define(_CLIENT_VERSION_MAJOR, $MAJOR)" "$CONFIGURE_FILE" || fail=1
grep -q "^define(_CLIENT_VERSION_MINOR, $MINOR)" "$CONFIGURE_FILE" || fail=1
grep -q "^define(_CLIENT_VERSION_REVISION, $REV)" "$CONFIGURE_FILE" || fail=1
grep -q "^#define CLIENT_VERSION_MINOR $MINOR" "$MSVC_FILE" || fail=1
for file in "${MANPAGES[@]}"; do
    grep -q "v${FULL_VERSION}" "$file" || { echo "  $file: version not found" >&2; fail=1; }
done
if [ "$fail" -ne 0 ]; then
    echo "Error: the files were not updated as expected: check with git diff." >&2
    exit 1
fi

echo "========================================================"
echo "Done. Updated files:"
printf ' - %s\n' "${TARGET_FILES[@]}"
echo "Check with 'git diff', then commit."
echo "========================================================"
