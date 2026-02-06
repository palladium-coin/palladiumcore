#!/usr/bin/env bash
export LC_ALL=C

###############################################################################
# Quick Build Script for Palladium Core
# Automates dependencies installation, BerkeleyDB setup, and compilation
###############################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    log_error "Do not run this script as root (sudo will be called when needed)"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

###############################################################################
# STEP 1: Install Dependencies
###############################################################################
install_dependencies() {
    log_info "Installing build dependencies..."

    sudo apt update

    # Essential build tools
    log_info "Installing essential build tools..."
    sudo apt install -y \
        build-essential \
        libtool \
        autotools-dev \
        automake \
        pkg-config \
        bsdmainutils \
        python3 \
        git \
        curl

    # Palladium core dependencies
    log_info "Installing Palladium core dependencies..."
    sudo apt install -y \
        libevent-dev \
        libboost-system-dev \
        libboost-filesystem-dev \
        libboost-test-dev \
        libboost-thread-dev

    # Qt 5 for GUI
    log_info "Installing Qt5 for GUI..."
    sudo apt install -y \
        libqt5gui5 \
        libqt5core5a \
        libqt5dbus5 \
        qttools5-dev \
        qttools5-dev-tools \
        qtbase5-dev

    # Optional dependencies
    log_info "Installing optional dependencies..."
    sudo apt install -y \
        libqrencode-dev \
        libzmq3-dev \
        libminiupnpc-dev

    # Debugging tools
    log_info "Installing debugging tools..."
    sudo apt install -y gdb valgrind

    # ccache for faster rebuilds
    log_info "Installing ccache for faster rebuilds..."
    sudo apt install -y ccache

    log_info "Dependencies installed successfully!"
}

###############################################################################
# STEP 2: Install BerkeleyDB 4.8
###############################################################################
install_berkeleydb() {
    log_info "Installing BerkeleyDB 4.8..."

    if [ -d "$SCRIPT_DIR/db4" ]; then
        log_warn "BerkeleyDB directory already exists. Skipping installation."
        log_warn "Remove db4/ folder to reinstall."
        return 0
    fi

    if [ ! -f "./contrib/install_db4.sh" ]; then
        log_error "install_db4.sh script not found in contrib/"
        exit 1
    fi

    ./contrib/install_db4.sh "$SCRIPT_DIR"

    log_info "BerkeleyDB 4.8 installed successfully!"
}

###############################################################################
# STEP 3: Build Palladium Core
###############################################################################
build_palladium() {
    log_info "Starting Palladium Core build process..."

    # Clean previous build if Makefile exists
    if [ -f "Makefile" ]; then
        log_info "Cleaning previous build configuration..."
        make distclean 2>/dev/null || true
    fi

    # Always regenerate build files to ensure subdirectories (secp256k1, univalue) are in sync
    log_info "Regenerating build files (autogen.sh)..."
    ./autogen.sh

    # Set BerkeleyDB prefix
    export BDB_PREFIX="$SCRIPT_DIR/db4"

    if [ ! -d "$BDB_PREFIX" ]; then
        log_error "BerkeleyDB not found at $BDB_PREFIX"
        log_error "Please run the script with --install-deps first"
        exit 1
    fi

    # Setup ccache for faster rebuilds
    if command -v ccache &> /dev/null; then
        log_info "Enabling ccache for faster compilation..."
        # Add ccache to PATH (it will wrap gcc/g++ automatically)
        export PATH="/usr/lib/ccache:$PATH"
        # Use plain gcc/g++ names (ccache wrappers will intercept them)
        export CC="gcc"
        export CXX="g++"
        ccache -z  # Zero statistics before build
    else
        log_warn "ccache not found, using plain gcc/g++..."
        export CC="gcc"
        export CXX="g++"
    fi

    # Calculate optimal number of parallel jobs
    JOBS=$(nproc)

    log_info "Available CPU cores: $JOBS"
    log_info "Using $JOBS parallel jobs"

    # Configure build with optimization flags
    log_info "Configuring build with Qt5 and BerkeleyDB 4.8..."
    ./configure \
        --enable-gui=qt5 \
        --with-gui=qt5 \
        --enable-zmq \
        --with-miniupnpc \
        --disable-tests \
        --disable-bench \
        CXXFLAGS="-O2 -pipe" \
        BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" \
        BDB_CFLAGS="-I${BDB_PREFIX}/include"

    # Compile with parallel jobs
    log_info "Starting parallel compilation..."
    START_TIME=$(date +%s)

    # Use make with parallel jobs
    make -j"$JOBS"

    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))

    log_info "Build completed successfully in ${BUILD_TIME}s!"

    # Show ccache stats after build
    if command -v ccache &> /dev/null; then
        log_info "ccache statistics:"
        ccache -s | head -n 10
    fi

    log_info "Binaries location:"
    log_info "  - palladiumd: $SCRIPT_DIR/src/palladiumd"
    log_info "  - palladium-cli: $SCRIPT_DIR/src/palladium-cli"
    log_info "  - palladium-qt: $SCRIPT_DIR/src/qt/palladium-qt"
}

###############################################################################
# STEP 4: Clean build artifacts
###############################################################################
clean_build() {
    log_info "Cleaning build artifacts..."

    if [ -f "Makefile" ]; then
        make clean
        log_info "Build artifacts cleaned"
    else
        log_warn "No Makefile found, nothing to clean"
    fi

    if [ -f "configure" ]; then
        log_info "Removing configure script..."
        make distclean 2>/dev/null || true
    fi
}

###############################################################################
# STEP 5: Quick rebuild (without reconfigure)
###############################################################################
rebuild_palladium() {
    log_info "Quick rebuild (keeping existing configuration)..."

    if [ ! -f "Makefile" ]; then
        log_error "No Makefile found. Run --build first to configure."
        exit 1
    fi

    # Setup ccache for faster rebuilds
    if command -v ccache &> /dev/null; then
        log_info "Enabling ccache for faster compilation..."
        # Add ccache to PATH (it will wrap gcc/g++ automatically)
        export PATH="/usr/lib/ccache:$PATH"
        # Use plain gcc/g++ names (ccache wrappers will intercept them)
        export CC="gcc"
        export CXX="g++"
        ccache -z  # Zero statistics before build
    else
        log_warn "ccache not found, using plain gcc/g++..."
        export CC="gcc"
        export CXX="g++"
    fi

    # Calculate optimal number of parallel jobs
    JOBS=$(nproc)

    log_info "Available CPU cores: $JOBS"
    log_info "Using $JOBS parallel jobs"

    # Clean only object files (fast)
    make clean

    # Compile with parallel jobs
    log_info "Starting parallel compilation..."
    START_TIME=$(date +%s)

    make -j"$JOBS"

    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))

    log_info "Rebuild completed successfully in ${BUILD_TIME}s!"

    # Show ccache stats after rebuild
    if command -v ccache &> /dev/null; then
        log_info "ccache statistics:"
        ccache -s | head -n 10
    fi

    log_info "Binaries location:"
    log_info "  - palladiumd: $SCRIPT_DIR/src/palladiumd"
    log_info "  - palladium-cli: $SCRIPT_DIR/src/palladium-cli"
    log_info "  - palladium-qt: $SCRIPT_DIR/src/qt/palladium-qt"
}

###############################################################################
# Main script
###############################################################################
show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Quick build script for Palladium Core

OPTIONS:
    --help              Show this help message
    --install-deps      Install system dependencies only
    --install-db        Install BerkeleyDB 4.8 only
    --build             Full build with reconfigure (distclean + configure + make)
    --rebuild           Quick rebuild without reconfigure (clean + make)
    --clean             Clean build artifacts
    --full              Full build (deps + db + build) [DEFAULT]

EXAMPLES:
    $0                  # Full build with all steps
    $0 --build          # Full rebuild (reconfigures everything)
    $0 --rebuild        # Quick rebuild (keeps configuration, faster for testing)
    $0 --clean          # Clean build artifacts
    $0 --install-deps   # Install dependencies only

EOF
}

# Parse command line arguments
case "${1:-}" in
    --help)
        show_help
        exit 0
        ;;
    --install-deps)
        install_dependencies
        ;;
    --install-db)
        install_berkeleydb
        ;;
    --build)
        build_palladium
        ;;
    --rebuild)
        rebuild_palladium
        ;;
    --clean)
        clean_build
        ;;
    --full|"")
        log_info "Starting full build process..."
        install_dependencies
        install_berkeleydb
        build_palladium
        log_info "Full build process completed!"
        ;;
    *)
        log_error "Unknown option: $1"
        show_help
        exit 1
        ;;
esac

exit 0
