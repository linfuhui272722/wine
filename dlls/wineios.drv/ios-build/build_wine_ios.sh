#!/bin/bash
# Wine iOS Cross-Compilation Build Script
# This script builds Wine libraries for iOS ARM64
#
# Usage: ./build_wine_ios.sh [options]
#
# Options:
#   --clean          Clean all build artifacts
#   --sdk <path>     Path to iOS SDK (default: ~/Library/Developer/Xcode/DerivedData/SDKs)
#   --jobs <n>       Number of parallel jobs (default: $(sysctl -n hw.ncpu))
#   --target <ver>   Target iOS version (default: 13.0)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WINE_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
INSTALL_DIR="$SCRIPT_DIR/install"
SDK_PATH=""
TARGET_VERSION="13.0"
JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
CLEAN=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=1
            shift
            ;;
        --sdk)
            SDK_PATH="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --target)
            TARGET_VERSION="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Detect macOS
if [[ "$(uname)" != "Darwin" ]]; then
    log_warn "This script is designed for macOS. Running in cross-compile mode."
    IS_CROSS_COMPILE=1
else
    IS_CROSS_COMPILE=0
fi

# Find Xcode SDK
find_sdk() {
    if [[ -n "$SDK_PATH" && -d "$SDK_PATH" ]]; then
        echo "$SDK_PATH"
        return
    fi
    
    local candidate_paths=(
        ~/Library/Developer/Xcode/DerivedData/SDKs/iPhoneOS*.sdk
        /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS*.sdk
        /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS*.sdk
    )
    
    for path in "${candidate_paths[@]}"; do
        local sdk=$(ls -d $path 2>/dev/null | head -1)
        if [[ -n "$sdk" && -d "$sdk" ]]; then
            echo "$sdk"
            return
        fi
    done
    
    log_error "iOS SDK not found. Please install Xcode and iOS SDK, or specify --sdk"
    exit 1
}

# Clean
clean() {
    log_info "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR"
    rm -rf "$INSTALL_DIR"
}

# Configure Wine for iOS
configure_wine() {
    log_info "Configuring Wine for iOS ARM64..."
    
    cd "$WINE_ROOT"
    
    # Clean previous configuration
    if [[ -f "Makefile" ]]; then
        make clean 2>/dev/null || true
    fi
    
    # Configure for cross-compilation to iOS
    local SDK="$(find_sdk)"
    local MIN_SDK_VERSION=$(echo "$TARGET_VERSION" | cut -d. -f1)
    
    log_info "Using SDK: $SDK"
    log_info "Target iOS version: $TARGET_VERSION"
    
    # Configure Wine
    # Note: Wine doesn't directly support iOS, but we configure for ARM64
    # and will build compatible libraries
    ./configure \
        --host=aarch64-apple-darwin \
        --enable-archs=aarch64 \
        --without-x \
        --without-mingw \
        --without-opengl \
        --without-gssapi \
        --disable-tests \
        --prefix="$INSTALL_DIR" \
        CFLAGS="-arch arm64 -miphoneos-version-min=$TARGET_VERSION -isysroot $SDK" \
        CXXFLAGS="-arch arm64 -miphoneos-version-min=$TARGET_VERSION -isysroot $SDK" \
        LDFLAGS="-arch arm64 -miphoneos-version-min=$TARGET_VERSION -isysroot $SDK" \
        2>&1 | tee "$BUILD_DIR/configure.log"
}

# Build Wine
build_wine() {
    log_info "Building Wine libraries..."
    
    cd "$WINE_ROOT"
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$INSTALL_DIR"
    
    # Build Wine using make
    # This builds the core Wine libraries
    make -j$JOBS 2>&1 | tee "$BUILD_DIR/build.log"
    
    log_info "Build completed!"
}

# Install Wine libraries
install_wine() {
    log_info "Installing Wine libraries to $INSTALL_DIR..."
    
    cd "$WINE_ROOT"
    make install 2>&1 | tee "$BUILD_DIR/install.log"
    
    log_info "Installing iOS-specific libraries..."
    
    # Copy iOS-specific libraries
    mkdir -p "$INSTALL_DIR/lib/wineios"
    mkdir -p "$INSTALL_DIR/lib/wine"
    
    # Find and copy all .dylib and .so files
    find "$INSTALL_DIR/lib" -name "*.dylib" -o -name "*.so" 2>/dev/null | while read lib; do
        cp "$lib" "$INSTALL_DIR/lib/wineios/" 2>/dev/null || true
    done
    
    # Copy Wine DLLs
    if [[ -d "$INSTALL_DIR/lib/wine" ]]; then
        cp -r "$INSTALL_DIR/lib/wine/"* "$INSTALL_DIR/lib/wineios/" 2>/dev/null || true
    fi
    
    log_info "Installation completed!"
}

# Create iOS static library
create_static_lib() {
    log_info "Creating static library for iOS..."
    
    local STATIC_LIB="$SCRIPT_DIR/libwineios.a"
    
    cd "$WINE_ROOT"
    
    # Create list of object files
    find . -name "*.o" -path "*/wine*" 2>/dev/null > "$BUILD_DIR/objects.txt"
    
    # Create static library
    ar rcs "$STATIC_LIB" $(cat "$BUILD_DIR/objects.txt" 2>/dev/null || echo "")
    
    if [[ -f "$STATIC_LIB" ]]; then
        log_info "Created $STATIC_LIB"
        ls -la "$STATIC_LIB"
    else
        log_warn "Static library creation skipped (no object files found)"
    fi
}

# Generate linker script
generate_linker_script() {
    log_info "Generating linker script..."
    
    cat > "$SCRIPT_DIR/wineios.link" << 'LINKER_EOF'
INPUT(
    libwineios.a
    -lobjc
    -lUIKit
    -lFoundation
    -lQuartzCore
    -lOpenGLES
    -lCoreGraphics
    -framework SystemConfiguration
    -lz
    -lbz2
)

GROUP(
    -lgcc
    -lc
    -lm
    -lstdc++
)

OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
LINKER_EOF

    log_info "Generated wineios.link"
}

# Main
main() {
    echo ""
    echo "========================================"
    echo "   Wine iOS Cross-Compilation Build"
    echo "========================================"
    echo ""
    
    if [[ $CLEAN -eq 1 ]]; then
        clean
        log_info "Clean completed!"
        exit 0
    fi
    
    log_info "Wine root: $WINE_ROOT"
    log_info "Build directory: $BUILD_DIR"
    log_info "Install directory: $INSTALL_DIR"
    log_info "Jobs: $JOBS"
    echo ""
    
    mkdir -p "$BUILD_DIR"
    
    # Run build steps
    time configure_wine
    time build_wine
    time install_wine
    time generate_linker_script
    
    echo ""
    echo "========================================"
    echo "   Build Summary"
    echo "========================================"
    echo ""
    
    log_info "Installation directory: $INSTALL_DIR"
    log_info "Libraries installed:"
    find "$INSTALL_DIR/lib" -name "*.dylib" -o -name "*.so" 2>/dev/null | head -20
    
    echo ""
    log_info "Build completed successfully!"
    echo ""
    echo "Next steps:"
    echo "  1. Open $SCRIPT_DIR/Wine.xcodeproj in Xcode"
    echo "  2. Select your target device"
    echo "  3. Build and run"
    echo ""
}

main "$@"
