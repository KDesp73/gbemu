# Compiler and flags
CC = gcc
CFLAGS = -Wall -Iinclude -fPIC
LDFLAGS =

# Headless mode: skip SDL entirely
ifneq ($(HEADLESS),1)
    CFLAGS  += $(shell PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig pkg-config --cflags sdl3)
    LDFLAGS += $(shell PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig pkg-config --libs sdl3)
endif

# Directories
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = dist
PREFIX = /usr/local/bin

LIBRARY_NAME = emu
SO_NAME = lib$(LIBRARY_NAME).so
A_NAME = lib$(LIBRARY_NAME).a

# Target and version info
TARGET = $(LIBRARY_NAME)-cli
version_file = include/version.h
VERSION_MAJOR = $(shell sed -n -e 's/\#define VERSION_MAJOR \([0-9]*\)/\1/p' $(version_file))
VERSION_MINOR = $(shell sed -n -e 's/\#define VERSION_MINOR \([0-9]*\)/\1/p' $(version_file))
VERSION_PATCH = $(shell sed -n -e 's/\#define VERSION_PATCH \([0-9]*\)/\1/p' $(version_file))
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Determine the build type
ifeq ($(type), RELEASE)
	CFLAGS += -O3
else
	SANITIZERS = -fsanitize=address,undefined
	CFLAGS  += -DDEBUG -ggdb
	CFLAGS  += $(SANITIZERS)
	LDFLAGS += $(SANITIZERS)
endif

# Source and object files
SRC_FILES := $(shell find $(SRC_DIR) -name '*.c' ! -name 'main.c' ! -name 'frontend_sdl.c' ! -name 'frontend_headless.c' ! -name 'main_wasm.c' ! -name 'frontend_wasm.c')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

ifeq ($(HEADLESS),1)
    CFLAGS  += -DEMU_HEADLESS
    FRONTEND_SRC = $(SRC_DIR)/frontend_headless.c
else
    FRONTEND_SRC = $(SRC_DIR)/frontend_sdl.c
endif
