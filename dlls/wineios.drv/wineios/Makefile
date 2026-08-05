# Wine iOS Makefile
#
# Copyright 2024 Wine Project

CC = clang
CFLAGS = -Wall -Wextra -fPIC -I. -I..
LDFLAGS = -lpthread -ldl

# Source files
SRCS = ios_syscalls.c ios_graphics.c ios_pe.c wineios.c
OBJS = $(SRCS:.c=.o)

# Output
LIB = libwineios.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	ar rcs $@ $^
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

# Dependencies
ios_graphics.o: ios_graphics.h ios_syscalls.h
ios_pe.o: ios_pe.h ios_syscalls.h
wineios.o: wineios.h ios_syscalls.h ios_graphics.h ios_pe.h
