#!/bin/bash
# Complete macOS Build Script for Wine iOS App
# Run this script on macOS with Xcode installed to create the final IPA

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== Wine iOS Complete Build Script ==="
echo "This script creates the final IPA on macOS"
echo ""

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "ERROR: This script must be run on macOS with Xcode installed"
    echo "Current platform: $(uname)"
    exit 1
fi

# Check for Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: Xcode command line tools not found"
    echo "Please install Xcode from Mac App Store"
    exit 1
fi

# Clean previous build
echo "Cleaning previous build..."
rm -rf "$BUILD_DIR/Wine.app"
rm -rf "$BUILD_DIR/Wine.ipa"

# Build using xcodebuild
echo ""
echo "Building with Xcode..."
cd "$SCRIPT_DIR"

xcodebuild -project project.pbxproj \
    -scheme Wine \
    -configuration Release \
    -destination 'generic/platform=iOS' \
    CODE_SIGN_IDENTITY="-" \
    CODE_SIGNING_REQUIRED=NO \
    CODE_SIGNING_ALLOWED=NO \
    BUILD_LIBRARY_FOR_DISTRIBUTING=NO \
    clean build 2>&1 | tee "$BUILD_DIR/build.log"

# Find the built app
echo ""
echo "Looking for built app..."
if [ -d "$BUILD_DIR/Release-iphoneos/Wine.app" ]; then
    APP_PATH="$BUILD_DIR/Release-iphoneos/Wine.app"
elif [ -d "$BUILD_DIR/Build/Products/Release-iphoneos/Wine.app" ]; then
    APP_PATH="$BUILD_DIR/Build/Products/Release-iphoneos/Wine.app"
else
    # Try to find it
    APP_PATH=$(find "$BUILD_DIR" -name "Wine.app" -type d 2>/dev/null | head -1)
fi

if [ -z "$APP_PATH" ] || [ ! -d "$APP_PATH" ]; then
    echo "ERROR: Built app not found"
    echo "Check build log: $BUILD_DIR/build.log"
    exit 1
fi

echo "Found app at: $APP_PATH"

# Copy app to Payload directory
echo ""
echo "Copying app to Payload directory..."
rm -rf "$BUILD_DIR/Payload"
mkdir -p "$BUILD_DIR/Payload"
cp -R "$APP_PATH" "$BUILD_DIR/Payload/"

# Create IPA
echo ""
echo "Creating IPA..."
cd "$BUILD_DIR"
rm -f Wine.ipa
zip -r Wine.ipa Payload

# Verify IPA
if [ -f "Wine.ipa" ]; then
    echo ""
    echo "=== Build Complete ==="
    echo "IPA created: $BUILD_DIR/Wine.ipa"
    echo "IPA size: $(du -h Wine.ipa | cut -f1)"
    echo ""
    echo "To install on jailbroken iOS device:"
    echo "  1. Copy IPA to device"
    echo "  2. Use Cydia Impactor or ssh to install"
    echo ""
    echo "Example installation commands:"
    echo "  scp Wine.ipa root@your-iphone:/tmp/"
    echo "  ssh root@your-iphone"
    echo "  unzip /tmp/Wine.ipa -d /Applications/"
    echo "  killall SpringBoard"
else
    echo "ERROR: IPA creation failed"
    exit 1
fi
