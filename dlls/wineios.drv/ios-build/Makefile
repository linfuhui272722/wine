# Wine iOS Build Makefile
#
# This Makefile orchestrates the cross-compilation of Wine for iOS
# Run on macOS with Xcode installed
#
# Usage:
#   make configure    - Configure Wine for iOS
#   make build        - Build Wine libraries
#   make install      - Install to local directory
#   make clean        - Clean all artifacts
#   make all          - Full build pipeline
#   make check        - Verify build

.PHONY: all configure build install clean check

# Configuration
WINE_ROOT := $(shell cd $(dir $(lastword $(MAKEFILE_LIST)))../.. && pwd)
BUILD_DIR := $(dir $(lastword $(MAKEFILE_LIST)))build
INSTALL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))install
SDK_PATH ?= $(shell xcrun --sdk iphoneos --show-sdk-path 2>/dev/null || echo "")
TARGET_VERSION ?= 13.0
JOBS ?= $(shell sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Compiler settings
CLANG := clang
CLANGXX := clang++
SYSROOT := $(SDK_PATH)
TARGET := arm64-apple-ios

# Directories
IOS_APP_DIR := $(WINE_ROOT)/dlls/wineios.drv/ios-app
WINEIOS_DIR := $(WINE_ROOT)/dlls/wineios.drv/wineios

# Flags
COMMON_FLAGS := -target $(TARGET) -isysroot $(SYSROOT) -miphoneos-version-min=$(TARGET_VERSION)
CFLAGS := $(COMMON_FLAGS) -fPIC -Wall -Wextra -O2
CXXFLAGS := $(COMMON_FLAGS) -fPIC -Wall -Wextra -O2 -stdlib=libc++
LDFLAGS := $(COMMON_FLAGS) -stdlib=libc++

# Wine source directories
WINE_DLLS := advapi32 crypt32 gdi32 kernel32 user32 shell32 comctl32 comdlg32
WINE_DLLS += version winmm winsock ws2_32 ole32 oleaut32 rpcrt4 shdocvw urlmon
WINE_DLLS += msi msacm32 wininet cabinet dnsapi ddraw dsound dxguid
WINE_LIBS := wine wineboot winecfg wineserver

# Default target
all: configure build install

configure:
	@echo "=== Configuring Wine for iOS ==="
	@echo "Wine Root: $(WINE_ROOT)"
	@echo "SDK Path: $(SYSROOT)"
	@echo "Target: $(TARGET)"
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(INSTALL_DIR)
	
	# Configure Wine build system
	cd $(WINE_ROOT) && \
		./configure \
		--host=aarch64-apple-darwin \
		--enable-archs=aarch64 \
		--without-x \
		--without-mingw \
		--without-opengl \
		--disable-tests \
		--disable-win16 \
		CC="$(CLANG)" \
		CXX="$(CLANGXX)" \
		CFLAGS="$(CFLAGS)" \
		CXXFLAGS="$(CXXFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		2>&1 | tee $(BUILD_DIR)/configure.log || true

build:
	@echo "=== Building Wine for iOS ==="
	cd $(WINE_ROOT) && make -j$(JOBS) 2>&1 | tee $(BUILD_DIR)/build.log

install:
	@echo "=== Installing Wine to $(INSTALL_DIR) ==="
	cd $(WINE_ROOT) && make install prefix=$(INSTALL_DIR) 2>&1 | tee $(BUILD_DIR)/install.log
	
	# Create iOS-specific structure
	@mkdir -p $(INSTALL_DIR)/lib/wineios
	@mkdir -p $(INSTALL_DIR)/share/wine
	
	# Copy and organize Wine libraries
	@if [ -d "$(INSTALL_DIR)/lib/wine" ]; then \
		cp -r $(INSTALL_DIR)/lib/wine/* $(INSTALL_DIR)/lib/wineios/; \
	fi
	
	# Copy built libraries
	@for lib in $(INSTALL_DIR)/lib/*.dylib $(INSTALL_DIR)/lib/*.so 2>/dev/null; do \
		[ -f "$$lib" ] && cp "$$lib" $(INSTALL_DIR)/lib/wineios/; \
	done
	
	@echo "=== Creating iOS library bundle ==="
	
	# Create combined static library
	@echo "Creating libwineios.a..."
	
	# Collect all Wine object files
	@find $(WINE_ROOT) -name "*.o" -path "*wine*" 2>/dev/null > $(BUILD_DIR)/wine_objects.txt || true
	
	# Create libwineios.a
	@ar rcs $(INSTALL_DIR)/lib/wineios.a $$(cat $(BUILD_DIR)/wine_objects.txt 2>/dev/null) || true
	
	# Copy iOS-specific code
	@cp $(WINEIOS_DIR)/*.c $(INSTALL_DIR)/lib/wineios/ 2>/dev/null || true
	@cp $(WINEIOS_DIR)/*.h $(INSTALL_DIR)/lib/wineios/ 2>/dev/null || true
	
	@echo "Installation complete!"

clean:
	@echo "=== Cleaning Wine build ==="
	cd $(WINE_ROOT) && make clean 2>/dev/null || true
	rm -rf $(BUILD_DIR)
	rm -rf $(INSTALL_DIR)
	@echo "Clean complete!"

check:
	@echo "=== Checking build configuration ==="
	@echo "Wine Root: $(WINE_ROOT)"
	@echo "SDK Path: $(SDK_PATH)"
	@echo "Xcode version:"
	@xcodebuild -version 2>/dev/null || echo "Xcode not found"
	@echo ""
	@echo "Available iOS SDKs:"
	@ls -la /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/ 2>/dev/null || \
		ls -la ~/Library/Developer/Xcode/DerivedData/SDKs/ 2>/dev/null || \
		echo "No SDKs found"
	@echo ""
	@echo "Checking Wine source:"
	@ls -la $(WINE_ROOT)/dlls/kernel32/*.c 2>/dev/null | head -5 || echo "Wine source not found"

# iOS App specific targets
build-ios-app:
	@echo "=== Building iOS App ==="
	cd $(IOS_APP_DIR) && \
		xcodebuild -project Wine.xcodeproj \
		-sdk iphoneos \
		-configuration Release \
		CODE_SIGN_IDENTITY="" \
		CODE_SIGNING_REQUIRED=NO \
		CODE_SIGNING_ALLOWED=NO \
		IPHONEOS_DEPLOYMENT_TARGET=$(TARGET_VERSION) \
		build

package-ipa:
	@echo "=== Packaging IPA ==="
	cd $(IOS_APP_DIR) && \
		xcodebuild -project Wine.xcodeproj \
		-sdk iphoneos \
		-configuration Release \
		CODE_SIGN_IDENTITY="" \
		CODE_SIGNING_REQUIRED=NO \
		CODE_SIGNING_ALLOWED=NO \
		IPHONEOS_DEPLOYMENT_TARGET=$(TARGET_VERSION) \
		build \
		-exportArchive \
		-archivePath build/Release-iphoneos/Wine.xcarchive \
		-exportOptionsPlist exportOptions.plist \
		-exportPath build/

.PHONY: build-ios-app package-ipa
