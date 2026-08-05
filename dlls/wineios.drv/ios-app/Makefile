# Makefile for Wine iOS Cross-Compilation
# This Makefile compiles the iOS app using clang with the theos SDK

# Configuration
SDK_PATH ?= /workspace/project/ios-sdk
TARGET_SDK ?= iPhoneOS16.5.sdk
MIN_IOS_VERSION ?= 13.0
PROJECT_NAME ?= Wine
BUILD_DIR ?= build

# Compiler settings
CC := clang
CXX := clang++
COMPILE_FLAGS := -target arm64-apple-ios \
                 -isysroot $(SDK_PATH)/$(TARGET_SDK) \
                 -miphoneos-version-min=$(MIN_IOS_VERSION) \
                 -fobjc-arc \
                 -DNS_BLOCKS_ASSERTED=YES \
                 -Wall -Wextra

# Source files
OBJS := $(BUILD_DIR)/main.o \
         $(BUILD_DIR)/WineAppDelegate.o \
         $(BUILD_DIR)/WineViewController.o \
         $(BUILD_DIR)/WineEventQueue.o \
         $(BUILD_DIR)/WineBridge.o \
         $(BUILD_DIR)/WineJIT.o

# Default target
.PHONY: all
all: compile ipa-structure

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/Payload
	mkdir -p $(BUILD_DIR)/Sources

# Compile main.m
$(BUILD_DIR)/main.o: main.m | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile WineAppDelegate
$(BUILD_DIR)/WineAppDelegate.o: WineAppDelegate.m WineAppDelegate.h | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile WineViewController
$(BUILD_DIR)/WineViewController.o: WineViewController.m WineViewController.h | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile WineEventQueue
$(BUILD_DIR)/WineEventQueue.o: WineEventQueue.m WineEventQueue.h | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile WineBridge
$(BUILD_DIR)/WineBridge.o: WineBridge.m WineBridge.h | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile WineJIT
$(BUILD_DIR)/WineJIT.o: WineJIT.m WineJIT.h | $(BUILD_DIR)
	$(CC) $(COMPILE_FLAGS) -c $< -o $@

# Compile all sources
.PHONY: compile
compile: $(OBJS)
	@echo "Compilation complete!"
	@echo "Object files:"
	@ls -la $(OBJS)

# Note: Linking requires macOS due to Mach-O format
# Use build_on_mac.sh on macOS to complete the build

# Create IPA structure (for macOS completion)
.PHONY: ipa-structure
ipa-structure: $(OBJS)
	@echo "Creating IPA structure..."
	@cp Info.plist $(BUILD_DIR)/Payload/
	@cp Wine.entitlements $(BUILD_DIR)/Payload/
	@cp LaunchScreen.storyboard $(BUILD_DIR)/Payload/
	@mkdir -p $(BUILD_DIR)/Sources/
	@cp *.m *.h $(BUILD_DIR)/Sources/
	@echo ""
	@echo "=== Linux Cross-Compilation Complete ==="
	@echo "Object files: $(BUILD_DIR)/"
	@echo ""
	@echo "To create the final IPA:"
	@echo "1. Copy this project to a macOS machine"
	@echo "2. Run: ./build_on_mac.sh"
	@echo ""
	@echo "Or manually in Xcode:"
	@echo "  xcodebuild -project project.pbxproj -scheme Wine -configuration Release"

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# List object files
.PHONY: list
list:
	@ls -la $(OBJS)

.PHONY: help
help:
	@echo "Wine iOS Cross-Compilation Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Compile all sources and create IPA structure (default)"
	@echo "  compile      - Compile all sources to object files"
	@echo "  ipa-structure - Create IPA directory structure"
	@echo "  clean        - Remove build artifacts"
	@echo "  list         - List compiled object files"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  SDK_PATH     - Path to iOS SDK (default: /workspace/project/ios-sdk)"
	@echo "  TARGET_SDK   - Target SDK name (default: iPhoneOS16.5.sdk)"
	@echo "  MIN_IOS_VERSION - Minimum iOS version (default: 13.0)"
	@echo ""
	@echo "Example:"
	@echo "  make SDK_PATH=~/ios-sdk all"
