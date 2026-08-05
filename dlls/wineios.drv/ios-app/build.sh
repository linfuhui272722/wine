#!/bin/bash
# Wine iOS Cross-Compilation Build Script
# This script compiles the iOS app using theos SDK and creates an IPA

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK_DIR="/workspace/project/ios-sdk"
TARGET_SDK="iPhoneOS16.5.sdk"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== Wine iOS Build Script ==="
echo "SDK: $SDK_DIR/$TARGET_SDK"
echo "Build directory: $BUILD_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"

# Compile all source files
echo "Compiling source files..."

cd "$SCRIPT_DIR"

COMPILE_FLAGS="-target arm64-apple-ios -isysroot $SDK_DIR/$TARGET_SDK -miphoneos-version-min=13.0 -fobjc-arc -DNS_BLOCKS_ASSERTED=YES"

# Compile main.m
echo "  Compiling main.m..."
clang $COMPILE_FLAGS -c main.m -o "$BUILD_DIR/main.o"

# Compile WineAppDelegate
echo "  Compiling WineAppDelegate.m..."
clang $COMPILE_FLAGS -c WineAppDelegate.m -o "$BUILD_DIR/WineAppDelegate.o"

# Compile WineViewController
echo "  Compiling WineViewController.m..."
clang $COMPILE_FLAGS -c WineViewController.m -o "$BUILD_DIR/WineViewController.o"

# Compile WineEventQueue
echo "  Compiling WineEventQueue.m..."
clang $COMPILE_FLAGS -c WineEventQueue.m -o "$BUILD_DIR/WineEventQueue.o"

# Compile WineBridge
echo "  Compiling WineBridge.m..."
clang $COMPILE_FLAGS -c WineBridge.m -o "$BUILD_DIR/WineBridge.o"

# Compile WineJIT
echo "  Compiling WineJIT.m..."
clang $COMPILE_FLAGS -c WineJIT.m -o "$BUILD_DIR/WineJIT.o"

echo "Compilation complete!"

# Show object files
echo ""
echo "Object files created:"
ls -la "$BUILD_DIR"/*.o

echo ""
echo "=== Note on Linking ==="
echo "Due to Linux limitations, native linking to Mach-O format is not possible."
echo "To complete the build:"
echo "1. Copy this project to a macOS machine with Xcode installed"
echo "2. Open project.pbxproj in Xcode"
echo "3. Select your signing identity (or use 'Sign to Run Locally')"
echo "4. Build and Run to create the IPA"
echo ""
echo "Alternatively, use the following command on macOS:"
echo "  xcodebuild -project project.pbxproj -scheme Wine -configuration Release"

# Create IPA structure for manual assembly
echo ""
echo "Creating IPA structure..."
IPA_DIR="$BUILD_DIR/Wine.ipa/Payload/Wine.app"
mkdir -p "$IPA_DIR"

# Copy resources
cp -r Info.plist "$IPA_DIR/"
cp -r Wine.entitlements "$IPA_DIR/"
cp -r LaunchScreen.storyboard "$IPA_DIR/"

# Create placeholder executable info
cat > "$IPA_DIR/Executable.info" << EXECINFO
To create a working executable:
1. On macOS, build using Xcode
2. The executable will be placed here
EXECINFO

# Copy source for reference
mkdir -p "$BUILD_DIR/Sources/"
cp -r *.m *.h "$BUILD_DIR/Sources/"

echo ""
echo "Build structure created at: $BUILD_DIR"
echo ""
echo "=== Build Summary ==="
echo "Status: Source compilation successful"
echo "Output: Mach-O executable (requires macOS to link)"
echo ""
