# Wine iOS Makefile
#
# Copyright 2024 Wine Project
#
# Usage: 
#   make SDK=iPhoneOS16.5        # Cross-compile for iOS
#   make                         # Native build (for testing on macOS)

# SDK selection
SDK = iPhoneOS16.5
SYSROOT = /workspace/project/ios-sdk/$(SDK).sdk

# Toolchain
CC = clang
AR = ar

# iOS cross-compilation flags
IOS_CFLAGS = -target arm64-apple-ios -isysroot $(SYSROOT)
IOS_CFLAGS += -Wall -Wextra -I.

# Source files for Wine iOS core library
CORE_SRCS = ios_syscalls.c ios_graphics.c ios_pe.c wineios.c
CORE_OBJS = $(CORE_SRCS:.c=.o)

# Output library
LIB = libwineios.a

.PHONY: all clean ios info

all: ios

# iOS ARM64 cross-compilation
ios: $(CORE_OBJS)
	$(AR) rcs $(LIB) $(CORE_OBJS)
	@echo "Built iOS library: $(LIB)"
	@echo "Architecture: arm64"
	@echo "SDK: $(SDK)"

# Wine iOS object files
ios_syscalls.o: ios_syscalls.c ios_syscalls.h
	$(CC) $(IOS_CFLAGS) -c $< -o $@

ios_graphics.o: ios_graphics.c ios_graphics.h ios_syscalls.h
	$(CC) $(IOS_CFLAGS) -c $< -o $@

ios_pe.o: ios_pe.c ios_pe.h ios_syscalls.h
	$(CC) $(IOS_CFLAGS) -c $< -o $@

wineios.o: wineios.c wineios.h ios_syscalls.h ios_graphics.h ios_pe.h
	$(CC) $(IOS_CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(CORE_OBJS) $(LIB)

# Info
info:
	@echo "Available SDKs:"
	@ls /workspace/project/ios-sdk/
	@echo ""
	@echo "Usage: make SDK=iPhoneOS16.5 ios"
